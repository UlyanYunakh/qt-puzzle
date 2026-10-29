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
    color: appBackgroundColor

    readonly property color panelColor: "#cc292f39"
    readonly property color primaryColor: "#4f9cff"
    readonly property color textColor: "#f2f5f8"

    component MenuButton: Button {
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
            color: appBackgroundColor

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 18

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 24
                    text: qsTr("Puzzle")
                    color: window.textColor
                    font.pixelSize: 52
                    font.bold: true
                }

                MenuButton {
                    text: qsTr("Start")
                    onClicked: stack.push(startMenu)
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
            color: appBackgroundColor
            property url selectedImage: ""

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

                        Button {
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

                        Button {
                            text: qsTr("Back")
                            onClicked: stack.pop()
                        }
                        Item { Layout.fillWidth: true }
                        Button {
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
                                    requestedRows: rowCount.value
                                })
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: settingsMenu

        Rectangle {
            color: appBackgroundColor

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 22

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Settings")
                    color: window.textColor
                    font.pixelSize: 38
                    font.bold: true
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("No settings yet")
                    color: "#aeb8c4"
                    font.pixelSize: 17
                }
                MenuButton {
                    text: qsTr("Back")
                    onClicked: stack.pop()
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
            property bool loadFailed: false
            property bool paused: false
            property bool completed: false
            property int elapsedSeconds: 0

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

            Timer {
                id: gameTimer
                interval: 1000
                repeat: true
                running: false
                onTriggered: game.elapsedSeconds++
            }

            PuzzleCanvas {
                id: canvas
                anchors.fill: parent
                enabled: !game.paused && !game.loadFailed

                Component.onCompleted: {
                    const loaded = game.requestedPieceCount > 0
                                   ? startPuzzleWithPieceCount(game.imageUrl,
                                                               game.requestedPieceCount)
                                   : startPuzzle(game.imageUrl,
                                                 game.requestedColumns,
                                                 game.requestedRows)
                    game.loadFailed = !loaded
                    if (loaded)
                        gameTimer.start()
                }

                onPuzzleCompleted: {
                    game.completed = true
                    gameTimer.stop()
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
                    Item { Layout.fillWidth: true }
                    Button {
                        text: qsTr("Pause")
                        enabled: !game.loadFailed
                        onClicked: game.openPauseMenu()
                    }
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
                    height: 280
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
                    Button {
                        Layout.alignment: Qt.AlignRight
                        text: qsTr("Back")
                        onClicked: stack.replace(startMenu)
                    }
                }
            }
        }
    }
}
