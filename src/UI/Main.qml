import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Puzzle.App

ApplicationWindow {
    id: window
    visible: true
    width: initialWindowWidth
    height: initialWindowHeight
    minimumWidth: 800
    minimumHeight: 560
    title: initialWindowTitle
    color: applicationSettings.backgroundColor

    readonly property color panelColor: "#cc292f39"
    readonly property color primaryColor: "#4f9cff"
    readonly property color textColor: "#f2f5f8"

    component SoundButton: Button {
        onClicked: puzzleSoundSystem.playButtonClick()
    }

    component ActionButton: SoundButton {
        id: actionControl
        implicitWidth: 130
        implicitHeight: 42
        leftPadding: 18
        rightPadding: 18
        font.pixelSize: 16
        font.weight: Font.DemiBold

        contentItem: Text {
            text: actionControl.text
            font: actionControl.font
            color: actionControl.enabled ? window.textColor : "#73808d"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: 8
            color: !actionControl.enabled ? "#252b34"
                   : actionControl.down ? "#2f6fbd"
                   : actionControl.highlighted ? window.primaryColor
                   : actionControl.hovered ? "#3a4655" : "#303945"
            border.width: 1
            border.color: actionControl.highlighted ? "#80b9ff"
                          : actionControl.hovered ? "#708297" : "#4a5665"
            opacity: actionControl.enabled ? 1.0 : 0.7

            Behavior on color { ColorAnimation { duration: 90 } }
        }
    }

    component MenuButton: SoundButton {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: 260
        Layout.preferredHeight: 52
        font.pixelSize: 18
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: mainMenu
    }

    Component {
        id: mainMenu

        Rectangle {
            color: applicationSettings.backgroundColor

            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - 48, 620)
                spacing: 18

                Label {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 24
                    text: qsTr("Everything puzzle")
                    color: window.textColor
                    font.pixelSize: 52
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                MenuButton {
                    text: qsTr("Start")
                    onClicked: stack.push(startMenu)
                }
                MenuButton {
                    text: qsTr("Load game")
                    onClicked: stack.push(loadMenu)
                }
                MenuButton {
                    text: qsTr("Setting")
                    onClicked: stack.push(settingsMenu)
                }
                MenuButton {
                    text: qsTr("Exit")
                    onClicked: Qt.quit()
                }
            }
        }
    }

    Component {
        id: startMenu

        Rectangle {
            id: startPage
            color: applicationSettings.backgroundColor
            property url selectedImage: ""
            property int selectedSaveSlot: 1

            FileDialog {
                id: imageDialog
                title: qsTr("Select puzzle image")
                nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.bmp *.webp)")]
                onAccepted: startPage.selectedImage = selectedFile
            }

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width - 48, 620)
                height: Math.min(parent.height - 48, 620)
                radius: 16
                color: window.panelColor

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 32
                    spacing: 16

                    Label {
                        text: qsTr("New puzzle")
                        color: window.textColor
                        font.pixelSize: 32
                        font.bold: true
                    }

                    Label {
                        text: qsTr("Image")
                        color: window.textColor
                        font.pixelSize: 17
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        ActionButton {
                            Layout.preferredWidth: 170
                            text: qsTr("Choose image…")
                            onClicked: imageDialog.open()
                        }
                        Label {
                            Layout.fillWidth: true
                            elide: Text.ElideMiddle
                            color: "#c8d0da"
                            text: startPage.selectedImage.toString().length > 0
                                  ? decodeURIComponent(startPage.selectedImage.toString()).split('/').pop()
                                  : qsTr("Built-in image")
                        }
                    }

                    Label {
                        text: qsTr("Number of pieces")
                        color: window.textColor
                        font.pixelSize: 17
                    }

                    ComboBox {
                        id: pieceCountBox
                        Layout.fillWidth: true
                        model: [qsTr("About 250"), qsTr("About 500"),
                            qsTr("About 1000"), qsTr("About 2000"), qsTr("Custom")]
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: pieceCountBox.currentIndex < 4
                        text: qsTr("The final count may differ slightly to keep pieces proportional.")
                        color: "#aeb8c4"
                        wrapMode: Text.WordWrap
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        rowSpacing: 10
                        columnSpacing: 16
                        visible: pieceCountBox.currentIndex === 4

                        Label { text: qsTr("Width (columns)"); color: window.textColor }
                        SpinBox {
                            id: columnCount
                            Layout.fillWidth: true
                            from: 2
                            to: 100
                            value: 20
                            editable: true
                        }
                        Label { text: qsTr("Height (rows)"); color: window.textColor }
                        SpinBox {
                            id: rowCount
                            Layout.fillWidth: true
                            from: 2
                            to: 100
                            value: 15
                            editable: true
                        }
                    }

                    Label {
                        text: qsTr("Save slot")
                        color: window.textColor
                        font.pixelSize: 17
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: [qsTr("Slot 1"), qsTr("Slot 2"), qsTr("Slot 3"),
                            qsTr("Slot 4"), qsTr("Slot 5"), qsTr("Slot 6"),
                            qsTr("Slot 7"), qsTr("Slot 8"), qsTr("Slot 9"),
                            qsTr("Slot 10")]
                        onCurrentIndexChanged: startPage.selectedSaveSlot = currentIndex + 1
                    }

                    Item { Layout.fillHeight: true }

                    Label {
                        visible: pieceCountBox.currentIndex === 4
                                 && columnCount.value * rowCount.value > 10000
                        text: qsTr("The custom puzzle is limited to 10,000 pieces.")
                        color: "#ff8a80"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 8

                        ActionButton {
                            text: qsTr("Back")
                            onClicked: stack.pop()
                        }
                        Item { Layout.fillWidth: true }
                        ActionButton {
                            text: qsTr("Start puzzle")
                            highlighted: true
                            enabled: pieceCountBox.currentIndex !== 4
                                     || columnCount.value * rowCount.value <= 10000
                            onClicked: {
                                const predefined = [250, 500, 1000, 2000]
                                stack.replace(gamePage, {
                                    imageUrl: startPage.selectedImage,
                                    requestedPieceCount: pieceCountBox.currentIndex < 4
                                                         ? predefined[pieceCountBox.currentIndex]
                                                         : 0,
                                    requestedColumns: columnCount.value,
                                    requestedRows: rowCount.value,
                                    saveSlot: startPage.selectedSaveSlot,
                                    loadExisting: false
                                })
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: loadMenu

        Rectangle {
            color: applicationSettings.backgroundColor

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 28
                spacing: 16

                Label {
                    text: qsTr("Load game")
                    color: window.textColor
                    font.pixelSize: 36
                    font.bold: true
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    Column {
                        width: Math.max(760, parent.width)
                        spacing: 12

                        Repeater {
                            model: 10
                            delegate: Rectangle {
                                required property int index
                                property var slotData: saveGameSystem.slotInfo(index + 1)
                                width: parent.width
                                height: 150
                                radius: 10
                                color: window.panelColor

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 14

                                    Label {
                                        Layout.preferredWidth: 68
                                        text: qsTr("Slot %1").arg(index + 1)
                                        color: window.textColor
                                        font.bold: true
                                    }
                                    Image {
                                        Layout.preferredWidth: 170
                                        Layout.fillHeight: true
                                        source: slotData.exists ? slotData.originalImage : ""
                                        fillMode: Image.PreserveAspectFit
                                        asynchronous: true
                                    }
                                    Image {
                                        Layout.preferredWidth: 220
                                        Layout.fillHeight: true
                                        source: slotData.exists ? slotData.statePreview : ""
                                        fillMode: Image.PreserveAspectFit
                                        asynchronous: true
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: slotData.exists
                                              ? qsTr("Saved %1").arg(slotData.savedAt)
                                              : qsTr("Empty")
                                        color: slotData.exists ? "#c8d0da" : "#73808d"
                                        wrapMode: Text.WordWrap
                                    }
                                    ActionButton {
                                        text: qsTr("Load")
                                        enabled: slotData.exists
                                        onClicked: stack.replace(gamePage, {
                                            saveSlot: index + 1,
                                            loadExisting: true
                                        })
                                    }
                                }
                            }
                        }
                    }
                }

                ActionButton {
                    text: qsTr("Back")
                    onClicked: stack.pop()
                }
            }
        }
    }

    Component {
        id: settingsMenu

        Rectangle {
            color: applicationSettings.backgroundColor

            ColorDialog {
                id: backgroundColorDialog
                title: qsTr("Choose background color")
                selectedColor: applicationSettings.backgroundColor
                onAccepted: applicationSettings.backgroundColor = selectedColor
            }

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width - 48, 520)
                height: Math.min(parent.height - 24, 560)
                radius: 16
                color: window.panelColor

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 10

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Settings")
                        color: window.textColor
                        font.pixelSize: 38
                        font.bold: true
                    }

                    Label {
                        text: qsTr("Gameplay effects volume: %1%").arg(
                                  Math.round(gameplayVolume.value * 100))
                        color: window.textColor
                        font.pixelSize: 17
                    }
                    Slider {
                        id: gameplayVolume
                        Layout.fillWidth: true
                        from: 0
                        to: 1
                        stepSize: 0.01
                        value: puzzleSoundSystem.gameplayEffectsVolume
                        onMoved: puzzleSoundSystem.gameplayEffectsVolume = value
                    }

                    Label {
                        text: qsTr("System sounds volume: %1%").arg(
                                  Math.round(systemVolume.value * 100))
                        color: window.textColor
                        font.pixelSize: 17
                    }
                    Slider {
                        id: systemVolume
                        Layout.fillWidth: true
                        from: 0
                        to: 1
                        stepSize: 0.01
                        value: puzzleSoundSystem.systemSoundsVolume
                        onMoved: puzzleSoundSystem.systemSoundsVolume = value
                    }

                    Label {
                        text: qsTr("Autosave interval")
                        color: window.textColor
                        font.pixelSize: 17
                    }
                    ComboBox {
                        id: autoSaveInterval
                        Layout.fillWidth: true
                        model: [qsTr("Every minute"), qsTr("Every 5 minutes"),
                            qsTr("Every 10 minutes")]
                        Component.onCompleted: currentIndex =
                            applicationSettings.autoSaveMinutes === 1 ? 0
                            : applicationSettings.autoSaveMinutes === 10 ? 2 : 1
                        onActivated: applicationSettings.autoSaveMinutes =
                            currentIndex === 0 ? 1 : currentIndex === 2 ? 10 : 5
                    }

                    Label {
                        text: qsTr("Background color")
                        color: window.textColor
                        font.pixelSize: 17
                    }
                    RowLayout {
                        Layout.fillWidth: true

                        Rectangle {
                            width: 42
                            height: 30
                            radius: 5
                            color: applicationSettings.backgroundColor
                            border.color: "#c8d0da"
                        }
                        ActionButton {
                            Layout.fillWidth: true
                            text: qsTr("Choose color")
                            onClicked: {
                                backgroundColorDialog.selectedColor =
                                        applicationSettings.backgroundColor
                                backgroundColorDialog.open()
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    ActionButton {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 260
                        text: qsTr("Reset all settings")
                        onClicked: {
                            puzzleSoundSystem.resetToDefaults()
                            applicationSettings.resetToDefaults()
                            autoSaveInterval.currentIndex = 1
                        }
                    }

                    MenuButton {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Back")
                        onClicked: stack.pop()
                    }
                }
            }
        }
    }

    Component {
        id: gamePage

        Item {
            id: game
            property url imageUrl
            property int requestedPieceCount
            property int requestedColumns
            property int requestedRows
            property int saveSlot: 1
            property bool loadExisting: false
            property bool loadFailed: false
            property bool paused: false
            property bool completed: false
            property bool showingOriginal: false
            property int elapsedSeconds: 0
            property string saveStatus: ""

            function formattedTime() {
                const hours = Math.floor(elapsedSeconds / 3600)
                const minutes = Math.floor((elapsedSeconds % 3600) / 60)
                const seconds = elapsedSeconds % 60
                function padded(value) { return value < 10 ? "0" + value : value }
                return padded(hours) + ":" + padded(minutes) + ":" + padded(seconds)
            }

            function openPauseMenu() {
                if (loadFailed)
                    return
                paused = true
                gameTimer.stop()
            }

            function continueGame() {
                paused = false
                if (!completed && !loadFailed)
                    gameTimer.start()
            }

            function saveCurrentGame(automatic) {
                if (loadFailed)
                    return
                saveStatus = automatic ? qsTr("Autosaving…") : qsTr("Saving…")
                const started = canvas.grabToImage(function(result) {
                    result.saveToFile(saveGameSystem.previewFilePath(saveSlot))
                    const saved = saveGameSystem.save(saveSlot, canvas.saveState(),
                                                      canvas.sourceImage, elapsedSeconds)
                    saveStatus = saved
                                 ? (automatic ? qsTr("Autosaved") : qsTr("Game saved"))
                                 : qsTr("Save failed")
                    saveIndicatorTimer.restart()
                })
                if (!started) {
                    saveStatus = qsTr("Save failed")
                    saveIndicatorTimer.restart()
                }
            }

            Timer {
                id: gameTimer
                interval: 1000
                repeat: true
                running: false
                onTriggered: game.elapsedSeconds++
            }

            Timer {
                interval: applicationSettings.autoSaveMinutes * 60000
                repeat: true
                running: !game.paused && !game.completed && !game.loadFailed
                onTriggered: game.saveCurrentGame(true)
            }

            Timer {
                id: saveIndicatorTimer
                interval: 2500
                onTriggered: game.saveStatus = ""
            }

            PuzzleCanvas {
                id: canvas
                anchors.fill: parent
                backgroundColor: applicationSettings.backgroundColor
                enabled: !game.paused && !game.loadFailed

                Component.onCompleted: {
                    let loaded = false
                    if (game.loadExisting) {
                        const savedGame = saveGameSystem.loadState(game.saveSlot)
                        game.elapsedSeconds = savedGame.elapsedSeconds || 0
                        game.imageUrl = savedGame.originalImage || ""
                        loaded = restoreState(savedGame.canvasState || {})
                    } else {
                        loaded = game.requestedPieceCount > 0
                                 ? startPuzzleWithPieceCount(game.imageUrl,
                                                             game.requestedPieceCount)
                                 : startPuzzle(game.imageUrl,
                                               game.requestedColumns,
                                               game.requestedRows)
                    }
                    game.loadFailed = !loaded
                    if (loaded)
                        gameTimer.start()
                }

                onPuzzleCompleted: {
                    game.completed = true
                    gameTimer.stop()
                }

                onPiecesPlaced: function(pieceIds, snapped) {
                    puzzleSoundSystem.playPieceRelease(snapped)
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 58
                color: "#b31d222b"
                z: 20

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 12

                    Label {
                        text: game.completed
                              ? qsTr("Completed: %1").arg(game.formattedTime())
                              : game.formattedTime()
                        color: game.completed ? "#7ee787" : window.textColor
                        font.pixelSize: 20
                        font.bold: true
                    }
                    Label {
                        visible: game.saveStatus.length > 0
                        text: game.saveStatus
                        color: game.saveStatus === qsTr("Save failed")
                               ? "#ff8a80" : "#7ee787"
                        font.pixelSize: 16
                    }
                    Item { Layout.fillWidth: true }
                    ActionButton {
                        Layout.preferredWidth: 170
                        text: game.showingOriginal ? qsTr("Hide image") : qsTr("Original image")
                        enabled: !game.loadFailed
                        onClicked: game.showingOriginal = !game.showingOriginal
                    }
                    ActionButton {
                        text: qsTr("Pause")
                        enabled: !game.loadFailed
                        onClicked: game.openPauseMenu()
                    }
                }
            }

            Rectangle {
                id: originalPreview
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: 18
                width: Math.min(280, parent.width * 0.25)
                height: width * 0.66
                radius: 8
                color: "#e61d222b"
                border.color: "#9aa4b2"
                border.width: 1
                clip: true
                visible: game.showingOriginal && !game.paused
                z: 30

                Image {
                    id: originalThumbnail
                    anchors.fill: parent
                    anchors.margins: 6
                    source: canvas.sourceImage
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    cache: true

                    readonly property real paintedLeft: (width - paintedWidth) / 2
                    readonly property real paintedTop: (height - paintedHeight) / 2
                }

                MouseArea {
                    id: previewHover
                    anchors.fill: parent
                    anchors.margins: 6
                    hoverEnabled: true
                    acceptedButtons: Qt.AllButtons
                }
            }

            Rectangle {
                id: originalMagnifier
                anchors.left: originalPreview.left
                anchors.bottom: originalPreview.top
                anchors.bottomMargin: 12
                width: Math.min(360, parent.width * 0.34)
                height: width * 0.72
                radius: 10
                color: "#1d222b"
                border.color: "#c9d1d9"
                border.width: 2
                clip: true
                z: 31

                readonly property bool overPaintedImage:
                    previewHover.mouseX >= originalThumbnail.paintedLeft
                    && previewHover.mouseX <= originalThumbnail.paintedLeft
                                               + originalThumbnail.paintedWidth
                    && previewHover.mouseY >= originalThumbnail.paintedTop
                    && previewHover.mouseY <= originalThumbnail.paintedTop
                                               + originalThumbnail.paintedHeight
                readonly property real normalizedX: Math.max(0, Math.min(1,
                    (previewHover.mouseX - originalThumbnail.paintedLeft)
                    / Math.max(1, originalThumbnail.paintedWidth)))
                readonly property real normalizedY: Math.max(0, Math.min(1,
                    (previewHover.mouseY - originalThumbnail.paintedTop)
                    / Math.max(1, originalThumbnail.paintedHeight)))
                readonly property real magnification: Math.max(
                    3.0,
                    width / Math.max(1, originalThumbnail.paintedWidth),
                    height / Math.max(1, originalThumbnail.paintedHeight))

                visible: originalPreview.visible
                         && previewHover.containsMouse
                         && overPaintedImage

                Image {
                    width: Math.max(1, originalThumbnail.paintedWidth
                                       * originalMagnifier.magnification)
                    height: Math.max(1, originalThumbnail.paintedHeight
                                        * originalMagnifier.magnification)
                    x: Math.max(originalMagnifier.width - width,
                                Math.min(0, originalMagnifier.width / 2
                                            - originalMagnifier.normalizedX * width))
                    y: Math.max(originalMagnifier.height - height,
                                Math.min(0, originalMagnifier.height / 2
                                            - originalMagnifier.normalizedY * height))
                    source: canvas.sourceImage
                    fillMode: Image.Stretch
                    asynchronous: true
                    cache: true
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: 6
                    height: 6
                    radius: 3
                    color: "transparent"
                    border.color: "#ffffff"
                    border.width: 1
                }
            }

            Rectangle {
                anchors.fill: parent
                color: "#b3000000"
                visible: game.paused
                z: 100

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.AllButtons
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: 380
                    height: 420
                    radius: 16
                    color: window.panelColor

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 18

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.bottomMargin: 10
                            text: qsTr("Paused")
                            color: window.textColor
                            font.pixelSize: 36
                            font.bold: true
                        }
                        MenuButton {
                            text: qsTr("Continue")
                            onClicked: game.continueGame()
                        }
                        MenuButton {
                            text: qsTr("Settings")
                            onClicked: stack.push(settingsMenu)
                        }
                        MenuButton {
                            text: qsTr("Save game")
                            onClicked: game.saveCurrentGame(false)
                        }
                        MenuButton {
                            text: qsTr("Exit to menu")
                            onClicked: stack.replace(mainMenu)
                        }
                    }
                }
            }

            Rectangle {
                anchors.centerIn: parent
                width: 420
                height: 180
                radius: 14
                color: window.panelColor
                visible: game.loadFailed
                z: 200

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("The selected image or puzzle size could not be loaded.")
                        color: window.textColor
                        wrapMode: Text.WordWrap
                    }
                    ActionButton {
                        Layout.alignment: Qt.AlignRight
                        text: qsTr("Back")
                        onClicked: stack.replace(startMenu)
                    }
                }
            }
        }
    }
}
