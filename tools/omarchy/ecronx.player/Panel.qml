import QtQuick
import QtQuick.Controls
import Quickshell
import Quickshell.Io
import qs.Commons
import qs.Commons as Commons
import qs.Ui

// mokaGram в баре: клиент сам отдаёт состояние, обложку, тексты и очередь через
// сокет $XDG_RUNTIME_DIR/mokagram/ctl.sock (см. StartShellBridge в клиенте).
Panel {
  id: root
  moduleName: "ecronx.player"
  ipcTarget: "ecronx.player"
  implicitWidth: button.implicitWidth + labelItem.width
  implicitHeight: button.implicitHeight

  // ---- состояние от клиента
  property bool linked: false
  property var st: ({ has: false })
  property var lyrics: ({ id: "", found: false, synced: false, lines: [] })
  property double basePos: 0
  property double baseAt: 0
  property double livePos: 0
  property string tab: "now"
  property var library: ({ liked: [], playlists: [] })
  property var comments: ({ id: "", list: [] })
  property string listId: ""
  property int sleepSel: 0
  property double sleepClickAt: 0
  property string lastNotified: ""
  property bool firstState: true
  readonly property var curList: {
    var P = library.playlists || []
    for (var i = 0; i < P.length; i++) if (P[i].id === listId) return P[i]
    return P.length > 0 ? P[0] : null
  }
  readonly property bool commentsHere: comments.id === st.id && (comments.list || []).length > 0

  readonly property bool has: linked && st.has === true
  readonly property bool playing: has && st.playing === true
  readonly property real lenMs: has ? (st.lenMs || 0) : 0
  readonly property color accent: (st.accent && st.accent !== "") ? Qt.color(st.accent) : root.bar.foreground
  readonly property bool lyricsHere: lyrics.id === st.id && lyrics.found && lyrics.lines.length > 0

  function send(line) {
    if (sock.connected) { sock.write(line + "\n"); sock.flush() }
  }
  function fmt(ms) {
    var s = Math.max(0, Math.floor(ms / 1000)), m = Math.floor(s / 60)
    s = s % 60
    return m + ":" + (s < 10 ? "0" : "") + s
  }
  function applyState(o) {
    st = o
    basePos = o.posMs || 0
    baseAt = Date.now()
    livePos = basePos
    if ((o.sleepLeft || 0) === 0 && sleepSel !== 0 && Date.now() - sleepClickAt > 1500) sleepSel = 0
    if (o.has && o.id !== lastNotified) {
      lastNotified = o.id
      if (!firstState && o.playing && !opened) {
        // виджет живёт на каждом баре: замок через mkdir, чтобы уведомление ушло одно
        var script = 'l="$XDG_RUNTIME_DIR/mokagram/notify.lock"; if mkdir "$l" 2>/dev/null; then notify-send "$@"; sleep 3; rmdir "$l"; fi'
        var cmd = ["sh", "-c", script, "_", "-a", "mokaGram"]
        if (o.cover) { cmd.push("-i"); cmd.push(o.cover) }
        cmd.push(o.title || "mokaGram"); cmd.push(o.artist || "")
        notif.command = cmd
        notif.running = true
      }
    }
    if (o.has !== undefined) firstState = false
  }
  Process { id: notif }
  readonly property int lineIdx: {
    if (!lyricsHere || !lyrics.synced) return -1
    var L = lyrics.lines, p = livePos + 150, r = -1
    for (var i = 0; i < L.length; i++) { if (L[i][0] <= p) r = i; else break }
    return r
  }

  Socket {
    id: sock
    path: Quickshell.env("XDG_RUNTIME_DIR") + "/mokagram/ctl.sock"
    connected: true
    onConnectedChanged: {
      root.linked = connected
      if (!connected) root.st = ({ has: false })
    }
    parser: SplitParser {
      onRead: function(data) {
        var o
        try { o = JSON.parse(data) } catch (e) { return }
        if (o.t === "state") root.applyState(o)
        else if (o.t === "lyrics") root.lyrics = o
        else if (o.t === "library") root.library = o
        else if (o.t === "comments") root.comments = o
      }
    }
  }
  // клиент мог быть запущен позже шелла: пробуем переподключиться
  Timer {
    interval: 3000
    repeat: true
    running: !sock.connected
    onTriggered: { sock.connected = false; sock.connected = true }
  }
  // плавный бегунок между пакетами состояния
  Timer {
    interval: 250
    repeat: true
    running: root.playing && root.opened
    onTriggered: root.livePos = Math.min(root.lenMs > 0 ? root.lenMs : 1e12, root.basePos + (Date.now() - root.baseAt))
  }

  BarIconButton {
    id: button
    anchors.left: parent.left
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    width: implicitWidth
    bar: root.bar
    text: root.playing ? "󰝚" : "󰝛"
    onPressed: function(b) {
      if (b === Qt.RightButton) root.send("toggle")
      else if (b === Qt.MiddleButton) root.send("next")
      else root.toggle()
    }
    onWheelMoved: function(delta) {
      var v = Math.max(0, Math.min(1, (root.st.volume || 0) + (delta > 0 ? 0.05 : -0.05)))
      root.send("volume " + v)
    }
  }

  // название трека в баре, бегущей строкой
  Item {
    id: labelItem
    anchors.left: button.right
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    readonly property bool shown: root.has && !root.bar.vertical && (root.st.title || "") !== ""
    readonly property real maxW: Style.space(190)
    // фиксированная ширина: иначе соседние иконки бара подпрыгивают при каждом треке
    width: shown ? maxW + Style.space(10) : 0
    visible: shown
    clip: true
    // название обычным цветом, исполнитель приглушённо, без разделителя-точки
    Row {
      id: strip
      spacing: Style.space(8)
      anchors.verticalCenter: parent.verticalCenter
      readonly property bool needsScroll: implicitWidth > labelItem.maxW
      x: 0
      Text {
        textFormat: Text.PlainText
        text: root.st.title || ""
        color: root.bar.barForeground
        font.family: root.bar.fontFamily
        font.pixelSize: Style.font.body
      }
      Text {
        visible: (root.st.artist || "") !== ""
        textFormat: Text.PlainText
        text: root.st.artist || ""
        color: Qt.darker(root.bar.barForeground, 1.6)
        font.family: root.bar.fontFamily
        font.pixelSize: Style.font.body
      }
      SequentialAnimation on x {
        running: strip.needsScroll && !root.opened
        loops: Animation.Infinite
        PauseAnimation { duration: 1800 }
        NumberAnimation {
          to: -(strip.implicitWidth - labelItem.maxW)
          duration: Math.max(1500, (strip.implicitWidth - labelItem.maxW) * 35)
          easing.type: Easing.InOutSine
        }
        PauseAnimation { duration: 1200 }
        NumberAnimation { to: 0; duration: 500; easing.type: Easing.InOutSine }
      }
      onImplicitWidthChanged: x = 0
    }
    MouseArea {
      anchors.fill: parent
      acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
      cursorShape: Qt.PointingHandCursor
      onClicked: function(m) {
        if (m.button === Qt.RightButton) root.send("toggle")
        else if (m.button === Qt.MiddleButton) root.send("next")
        else root.toggle()
      }
      onWheel: function(w) {
        var v = Math.max(0, Math.min(1, (root.st.volume || 0) + (w.angleDelta.y > 0 ? 0.05 : -0.05)))
        root.send("volume " + v)
      }
    }
  }

  KeyboardPanel {
    id: panel
    anchorItem: button
    owner: root
    bar: root.bar
    open: root.opened
    focusTarget: keyCatcher
    contentWidth: panel.fittedContentWidth(Style.space(440))
    // высота постоянная: не меняется от вкладки и загрузки текста/комментариев
    contentHeight: panel.fittedContentHeight(root.has ? Style.space(2000) : col.implicitHeight, Style.space(820))

    PanelKeyCatcher {
      id: keyCatcher
      anchors.fill: parent
      onMoveRequested: function(dx, dy) {
        if (dx !== 0) root.send("seek " + Math.max(0, root.livePos + dx * 5000))
        else if (dy !== 0) root.send("volume " + Math.max(0, Math.min(1, (root.st.volume || 0) - dy * 0.05)))
      }
      onActivateRequested: root.send("toggle")
      onCloseRequested: root.close()
      onTabRequested: function(direction) { root.switchPanel(direction) }
      onTextKey: function(t) {
        if (t === " ") root.send("toggle")
        else if (t === "n" || t === "N") root.send("next")
        else if (t === "p" || t === "P") root.send("prev")
        else if (t === "l" || t === "L") root.send("like")
        else if (t === "s" || t === "S") root.send("shuffle")
        else if (t === "r" || t === "R") root.send("repeat")
      }

      // Flickable вместо ScrollView: у него нет скроллбара, ширина контента не прыгает
      Flickable {
        id: scroller
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: col.implicitHeight
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds
        interactive: col.implicitHeight > height

        Column {
          id: col
          width: scroller.width
          spacing: Style.space(14)

          PanelHero {
            foreground: root.bar.foreground
            fontFamily: root.bar.fontFamily
            title: "mokaGram"
            meta: !root.linked ? "Client is not running" : !root.has ? "Nothing playing" : (root.st.source === "telegram" ? "Telegram audio" : "SoundCloud") + (root.st.together ? "  ·  Together " + (root.st.listeners || 0) : "")
            iconComponent: Component {
              Text {
                textFormat: Text.PlainText
                text: "󰝚"
                color: root.bar.foreground
                font.family: root.bar.fontFamily
                font.pixelSize: Style.font.display
                opacity: root.has ? 1 : 0.5
              }
            }
          }

          PanelSeparator { foreground: root.bar.foreground }

          ButtonGroup {
            visible: root.linked
            options: [{ value: "now", label: "Now" }, { value: "liked", label: "Liked" }, { value: "lists", label: "Playlists" }]
            value: root.tab
            foreground: root.bar.foreground
            background: root.bar.background
            fontFamily: root.bar.fontFamily
            fontSize: Style.font.bodySmall
            focusable: false
            onChanged: function(v) { root.tab = v }
          }

          // ---- библиотека: лайкнутые и плейлисты
          Column {
            id: libTab
            visible: root.linked && root.tab !== "now"
            width: parent.width
            spacing: Style.space(8)

            Flow {
              visible: root.tab === "lists"
              width: parent.width
              spacing: Style.space(6)
              Repeater {
                model: root.library.playlists || []
                Button {
                  required property var modelData
                  text: modelData.name
                  fontSize: Style.font.caption
                  foreground: root.bar.foreground
                  fontFamily: root.bar.fontFamily
                  bordered: true
                  selected: root.curList !== null && root.curList.id === modelData.id
                  onClicked: root.listId = modelData.id
                }
              }
            }

            ListView {
              id: libList
              ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOff; visible: false; width: 0 }
              width: parent.width
              height: Math.min(contentHeight, Style.space(460))
              clip: true
              boundsBehavior: Flickable.StopAtBounds
              model: root.tab === "liked" ? (root.library.liked || []) : (root.curList ? root.curList.tracks : [])
              delegate: CursorSurface {
                id: lrow
                required property var modelData
                required property int index
                width: libList.width
                height: Style.space(42)
                foreground: root.bar.foreground
                current: root.has && root.st.title === modelData.title && root.st.artist === modelData.artist
                hasCursor: lma.containsMouse
                Column {
                  anchors.verticalCenter: parent.verticalCenter
                  anchors.left: parent.left
                  anchors.leftMargin: Style.space(12)
                  anchors.right: parent.right
                  anchors.rightMargin: Style.space(12)
                  spacing: Style.space(1)
                  Text {
                    textFormat: Text.PlainText
                    text: lrow.modelData.title
                    width: parent.width
                    elide: Text.ElideRight
                    color: root.bar.foreground
                    font.family: root.bar.fontFamily
                    font.pixelSize: Style.font.bodySmall
                    font.bold: lrow.current
                  }
                  Text {
                    textFormat: Text.PlainText
                    text: lrow.modelData.artist
                    width: parent.width
                    elide: Text.ElideRight
                    color: Qt.darker(root.bar.foreground, 1.5)
                    font.family: root.bar.fontFamily
                    font.pixelSize: Style.font.caption
                  }
                }
                MouseArea {
                  id: lma
                  anchors.fill: parent
                  hoverEnabled: true
                  cursorShape: Qt.PointingHandCursor
                  onClicked: root.send(root.tab === "liked"
                    ? "playliked " + lrow.index
                    : "playlist " + root.curList.id + " " + lrow.index)
                }
              }
            }

            Text {
              visible: libList.count === 0
              width: parent.width
              horizontalAlignment: Text.AlignHCenter
              text: root.tab === "liked" ? "Лайкнутых треков пока нет" : "Плейлистов пока нет"
              color: Qt.darker(root.bar.foreground, 1.5)
              font.family: root.bar.fontFamily
              font.pixelSize: Style.font.body
            }
          }

          Column {
            id: nowTab
            visible: root.tab === "now"
            width: parent.width
            spacing: Style.space(14)

          // ---- обложка (во всю ширину) + название
          Item {
            visible: root.has
            width: parent.width
            height: visible ? Math.min(width, Style.space(300)) : 0

            BorderSurface {
              anchors.centerIn: parent
              width: parent.height
              height: parent.height
              radius: Style.spacing.labelGap
              color: Style.normalFillFor(root.bar.foreground, Commons.Color.accent)
              borderSpec: Border.controlSpec("normal", root.bar.foreground, Commons.Color.accent)
              Image {
                anchors.fill: parent
                anchors.margins: Style.space(2)
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: false
                source: root.has && root.st.cover ? "file://" + root.st.cover : ""
                visible: source !== ""
              }
              Text {
                anchors.centerIn: parent
                visible: !root.has || !root.st.cover
                text: "󰝚"
                color: root.bar.foreground
                font.family: root.bar.fontFamily
                font.pixelSize: Style.space(72)
                opacity: 0.5
              }
            }
          }

          Row {
            visible: root.has
            width: parent.width
            spacing: Style.space(10)
            Column {
              width: parent.width - heart.width - Style.space(10)
              spacing: Style.space(3)
              Text {
                textFormat: Text.PlainText
                text: root.st.title || ""
                width: parent.width
                elide: Text.ElideRight
                color: root.bar.foreground
                font.family: root.bar.fontFamily
                font.pixelSize: Style.font.title
                font.bold: true
              }
              Text {
                textFormat: Text.PlainText
                text: root.st.artist || ""
                width: parent.width
                elide: Text.ElideRight
                color: Qt.darker(root.bar.foreground, 1.3)
                font.family: root.bar.fontFamily
                font.pixelSize: Style.font.body
              }
            }
            Button {
              id: heart
              anchors.verticalCenter: parent.verticalCenter
              iconText: root.st.liked ? "󰋑" : "󰋕"
              width: Style.space(48)
              height: width
              iconSize: Style.space(26)
              foreground: root.bar.foreground
              fontFamily: root.bar.fontFamily
              active: root.st.liked === true
              enabled: root.st.canLike === true
              opacity: enabled ? 1 : 0.4
              onClicked: root.send("like")
            }
          }

          // ---- перемотка
          Column {
            visible: root.has && root.lenMs > 0
            width: parent.width
            spacing: Style.space(6)
            PanelSlider {
              id: seek
              bar: root.bar
              width: parent.width
              minimum: 0
              maximum: Math.max(1, root.lenMs)
              step: 1000
              value: root.livePos
              onReleased: function(v) { root.send("seek " + Math.round(v)) }
            }
            Item {
              width: parent.width
              height: t1.implicitHeight
              Text {
                id: t1
                text: root.fmt(seek.dragging ? seek.liveValue : root.livePos)
                color: Qt.darker(root.bar.foreground, 1.5)
                font.family: root.bar.fontFamily
                font.pixelSize: Style.font.caption
                font.bold: true
              }
              Text {
                anchors.right: parent.right
                text: root.fmt(root.lenMs)
                color: Qt.darker(root.bar.foreground, 1.5)
                font.family: root.bar.fontFamily
                font.pixelSize: Style.font.caption
                font.bold: true
              }
            }
          }

          // ---- управление: крупные иконки без рамок, одна квадратная кнопка play
          Row {
            id: ctl
            visible: root.has
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Style.space(10)
            readonly property var items: [
              { g: "󰒟", c: "shuffle", on: root.st.shuffle === true, ok: true, big: false },
              { g: "󰒮", c: "prev", on: false, ok: root.st.canPrev !== false, big: false },
              { g: root.playing ? "󰏤" : "󰐊", c: "toggle", on: false, ok: true, big: true },
              { g: "󰒭", c: "next", on: false, ok: root.st.canNext !== false, big: false },
              { g: root.st.repeat === "one" ? "󰑘" : "󰑖", c: "repeat", on: root.st.repeat !== "none", ok: true, big: false }
            ]
            Repeater {
              model: ctl.items
              Button {
                required property var modelData
                anchors.verticalCenter: parent.verticalCenter
                width: Style.space(modelData.big ? 76 : 56)
                height: width
                iconText: modelData.g
                iconSize: Style.space(modelData.big ? 36 : 28)
                foreground: root.bar.foreground
                fontFamily: root.bar.fontFamily
                bordered: modelData.big
                active: modelData.on
                enabled: modelData.ok
                opacity: modelData.ok ? 1 : 0.35
                onClicked: root.send(modelData.c)
              }
            }
          }

          // ---- громкость
          Column {
            visible: root.linked
            width: parent.width
            spacing: Style.space(6)
            Item {
              width: parent.width
              height: vh.implicitHeight
              PanelSectionHeader {
                id: vh
                text: "VOLUME"
                foreground: root.bar.foreground
                fontFamily: root.bar.fontFamily
              }
              Text {
                anchors.right: parent.right
                text: Math.round((vol.dragging ? vol.liveValue : (root.st.volume || 0)) * 100) + "%"
                color: Qt.darker(root.bar.foreground, 1.4)
                font.family: root.bar.fontFamily
                font.pixelSize: Style.font.caption
                font.bold: true
              }
            }
            PanelSlider {
              id: vol
              bar: root.bar
              width: parent.width
              minimum: 0
              maximum: 1
              step: 0.05
              value: root.st.volume || 0
              onMoved: function(v) { root.send("volume " + v) }
            }
          }

          // ---- скорость (tdesktop меняет её только у длинных треков)
          Column {
            visible: root.has && root.st.speedOk === true
            width: parent.width
            spacing: Style.space(6)
            PanelSectionHeader {
              text: "SPEED"
              foreground: root.bar.foreground
              fontFamily: root.bar.fontFamily
            }
            ButtonGroup {
              options: [{ value: "0.75", label: "0.75x" }, { value: "1", label: "1x" }, { value: "1.25", label: "1.25x" }, { value: "1.5", label: "1.5x" }, { value: "2", label: "2x" }]
              value: String(root.st.speed || 1)
              foreground: root.bar.foreground
              background: root.bar.background
              fontFamily: root.bar.fontFamily
              fontSize: Style.font.caption
              focusable: false
              onChanged: function(v) { root.send("speed " + v) }
            }
          }

          // ---- таймер сна
          Column {
            visible: root.has
            width: parent.width
            spacing: Style.space(6)
            Item {
              width: parent.width
              height: sh.implicitHeight
              PanelSectionHeader {
                id: sh
                text: "SLEEP TIMER"
                foreground: root.bar.foreground
                fontFamily: root.bar.fontFamily
              }
              Text {
                visible: (root.st.sleepLeft || 0) > 0
                anchors.right: parent.right
                text: root.fmt((root.st.sleepLeft || 0) * 1000)
                color: Qt.darker(root.bar.foreground, 1.4)
                font.family: root.bar.fontFamily
                font.pixelSize: Style.font.caption
                font.bold: true
              }
            }
            ButtonGroup {
              options: [{ value: "0", label: "Off" }, { value: "15", label: "15 min" }, { value: "30", label: "30 min" }, { value: "60", label: "60 min" }]
              value: String(root.sleepSel)
              foreground: root.bar.foreground
              background: root.bar.background
              fontFamily: root.bar.fontFamily
              fontSize: Style.font.caption
              focusable: false
              onChanged: function(v) {
                root.sleepSel = parseInt(v)
                root.sleepClickAt = Date.now()
                root.send("sleep " + v)
              }
            }
          }

          // ---- текст песни: текущая строка подсвечена, клик перематывает
          Column {
            visible: root.has && root.lyricsHere
            width: parent.width
            spacing: Style.space(6)
            PanelSeparator { foreground: root.bar.foreground }
            PanelSectionHeader {
              text: "LYRICS"
              foreground: root.bar.foreground
              fontFamily: root.bar.fontFamily
            }
            ListView {
              id: lyr
              ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOff; visible: false; width: 0 }
              width: parent.width
              height: Style.space(190)
              clip: true
              model: root.lyricsHere ? root.lyrics.lines : []
              currentIndex: root.lineIdx
              highlightRangeMode: root.lyrics.synced ? ListView.StrictlyEnforceRange : ListView.NoHighlightRange
              preferredHighlightBegin: height / 2 - Style.space(18)
              preferredHighlightEnd: height / 2 + Style.space(18)
              highlightMoveDuration: 280
              highlightFollowsCurrentItem: true
              boundsBehavior: Flickable.StopAtBounds
              delegate: Text {
                required property var modelData
                required property int index
                width: lyr.width
                textFormat: Text.PlainText
                text: modelData[1] === "" ? "·" : modelData[1]
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                topPadding: Style.space(5)
                bottomPadding: Style.space(5)
                color: root.bar.foreground
                opacity: !root.lyrics.synced || index === root.lineIdx ? 1 : 0.4
                font.family: root.bar.fontFamily
                font.pixelSize: index === root.lineIdx ? Style.font.title : Style.font.body
                font.bold: index === root.lineIdx
                Behavior on opacity { NumberAnimation { duration: 200 } }
                MouseArea {
                  anchors.fill: parent
                  enabled: root.lyrics.synced
                  cursorShape: Qt.PointingHandCursor
                  onClicked: root.send("seek " + modelData[0])
                }
              }
            }
          }

          // ---- комментарии SoundCloud по таймлайну, клик перематывает
          Column {
            visible: root.has && root.commentsHere
            width: parent.width
            spacing: Style.space(6)
            PanelSeparator { foreground: root.bar.foreground }
            PanelSectionHeader {
              text: "COMMENTS"
              foreground: root.bar.foreground
              fontFamily: root.bar.fontFamily
            }
            ListView {
              id: cmList
              ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOff; visible: false; width: 0 }
              width: parent.width
              height: Style.space(170)
              clip: true
              boundsBehavior: Flickable.StopAtBounds
              model: root.commentsHere ? root.comments.list : []
              delegate: CursorSurface {
                id: crow
                required property var modelData
                width: cmList.width
                height: cc.implicitHeight + Style.space(10)
                foreground: root.bar.foreground
                current: Math.abs(modelData[0] - root.livePos) < 5000
                hasCursor: cma.containsMouse
                Column {
                  id: cc
                  anchors.verticalCenter: parent.verticalCenter
                  anchors.left: parent.left
                  anchors.leftMargin: Style.space(12)
                  anchors.right: parent.right
                  anchors.rightMargin: Style.space(12)
                  spacing: Style.space(1)
                  Text {
                    textFormat: Text.PlainText
                    text: root.fmt(crow.modelData[0]) + "  " + crow.modelData[1]
                    width: parent.width
                    elide: Text.ElideRight
                    color: Qt.darker(root.bar.foreground, 1.5)
                    font.family: root.bar.fontFamily
                    font.pixelSize: Style.font.caption
                    font.bold: true
                  }
                  Text {
                    textFormat: Text.PlainText
                    text: crow.modelData[2]
                    width: parent.width
                    wrapMode: Text.WordWrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                    color: root.bar.foreground
                    font.family: root.bar.fontFamily
                    font.pixelSize: Style.font.bodySmall
                  }
                }
                MouseArea {
                  id: cma
                  anchors.fill: parent
                  hoverEnabled: true
                  cursorShape: Qt.PointingHandCursor
                  onClicked: root.send("seek " + crow.modelData[0])
                }
              }
            }
          }

          // ---- очередь
          Column {
            visible: root.has && (root.st.queue || []).length > 1
            width: parent.width
            spacing: Style.space(4)
            PanelSeparator { foreground: root.bar.foreground }
            PanelSectionHeader {
              text: "QUEUE"
              foreground: root.bar.foreground
              fontFamily: root.bar.fontFamily
            }
            Repeater {
              model: root.st.queue || []
              CursorSurface {
                id: qrow
                required property var modelData
                width: parent.width
                height: Style.space(42)
                foreground: root.bar.foreground
                current: modelData.current === true
                hasCursor: qma.containsMouse
                Column {
                  anchors.verticalCenter: parent.verticalCenter
                  anchors.left: parent.left
                  anchors.leftMargin: Style.space(12)
                  anchors.right: parent.right
                  anchors.rightMargin: Style.space(12)
                  spacing: Style.space(1)
                  Text {
                    textFormat: Text.PlainText
                    text: qrow.modelData.title
                    width: parent.width
                    elide: Text.ElideRight
                    color: root.bar.foreground
                    font.family: root.bar.fontFamily
                    font.pixelSize: Style.font.bodySmall
                    font.bold: qrow.modelData.current
                  }
                  Text {
                    textFormat: Text.PlainText
                    text: qrow.modelData.artist
                    width: parent.width
                    elide: Text.ElideRight
                    color: Qt.darker(root.bar.foreground, 1.5)
                    font.family: root.bar.fontFamily
                    font.pixelSize: Style.font.caption
                  }
                }
                MouseArea {
                  id: qma
                  anchors.fill: parent
                  hoverEnabled: true
                  cursorShape: Qt.PointingHandCursor
                  onClicked: root.send("playindex " + qrow.modelData.i)
                }
              }
            }
          }

          // ---- открыть в клиенте и поделиться
          Repeater {
            model: [
              [{ t: "Open player", i: "󰎆", c: "open", need: "" }, { t: "Fullscreen", i: "󰊓", c: "fullscreen", need: "" }],
              [{ t: "Copy link", i: "󰆏", c: "copylink", need: "link" }, { t: "SoundCloud", i: "󰖟", c: "openlink", need: "link" }, { t: "To chat", i: "󰒊", c: "forward", need: "" }]
            ]
            Row {
              id: shareRow
              required property var modelData
              visible: root.linked && (shareRow.modelData[0].need === "" || root.has)
              width: parent.width
              spacing: Style.space(6)
              Repeater {
                model: shareRow.modelData
                Button {
                  required property var modelData
                  visible: modelData.need === "" || root.st.link === true
                  width: (shareRow.width - shareRow.spacing * (shareRow.modelData.length - 1)) / shareRow.modelData.length
                  iconText: modelData.i
                  iconSize: Style.font.title
                  text: modelData.t
                  fontSize: Style.font.bodySmall
                  foreground: root.bar.foreground
                  fontFamily: root.bar.fontFamily
                  horizontalPadding: Style.spacing.controlPaddingX
                  verticalPadding: Style.spacing.controlPaddingY + Style.space(2)
                  bordered: true
                  onClicked: { root.send(modelData.c); root.close() }
                }
              }
            }
          }
          }

          Text {
            visible: !root.linked
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: "Запусти mokaGram, панель подключится сама"
            color: Qt.darker(root.bar.foreground, 1.5)
            font.family: root.bar.fontFamily
            font.pixelSize: Style.font.body
          }
        }
      }
    }
  }
}
