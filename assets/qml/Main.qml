import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: window
    visible: true
    minimumWidth: 300
    minimumHeight: 320
    width: Math.max(minimumWidth, contentColumn.implicitWidth + 32)
    height: Math.max(minimumHeight, contentColumn.implicitHeight + 32)
    title: "thinkfancpp"

    Material.theme: backend.darkMode ? Material.Dark : Material.Light
    Material.primary: backend.darkMode ? Material.color(Material.Grey, Material.Shade300)
                                       : Material.color(Material.Grey, Material.Shade800)
    Material.accent: Material.primary

    property bool exitPending: false
    property bool disengagedAcknowledged: false

    Timer {
        id: exitTimer
        interval: 3000
        repeat: false
        onTriggered: appShell.requestQuit()
    }

    onClosing: function(close) {
        if (appShell.hideToTrayOnClose()) {
            close.accepted = false
            return
        }
        if (!window.exitPending && backend.fanLevel !== "Auto" && !backend.fanLevel.includes("—")) {
            window.exitPending = true
            backend.showStatus("Fan level left at " + backend.fanLevel + " on exit")
            close.accepted = false
            exitTimer.start()
        }
    }

    readonly property color textColor: backend.themeTextColor
    readonly property color mutedColor: backend.themeMutedColor
    readonly property color warnBg: backend.themeWarningBackground
    readonly property color warnBorder: backend.themeWarningBorder
    readonly property color warnText: backend.themeWarningText
    readonly property color dangerText: backend.themeDangerButtonText
    readonly property color okColor: backend.themeSuccessColor
    readonly property color errorColor: backend.themeErrorColor

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 8

        GridLayout {
            columns: 2
            columnSpacing: 12
            rowSpacing: 6
            Layout.fillWidth: true

            Label { text: "CPU Temperature:"; color: window.textColor }
            Label { text: backend.cpuTemp; color: window.textColor; font.family: "monospace" }
            Label { text: "Fan RPM:"; color: window.textColor }
            Label { text: backend.fanRpm; color: window.textColor; font.family: "monospace" }
            Label { text: "Fan Level:"; color: window.textColor }
            Label { text: backend.fanLevel; color: window.textColor; font.family: "monospace" }
            Label { text: "Fan Control:"; color: window.textColor }
            Label {
                text: backend.readable ? (backend.fanEnabled ? "Enabled" : "Disabled") : "Unavailable"
                color: !backend.readable ? window.mutedColor : (backend.fanEnabled ? window.okColor : window.errorColor)
                font.family: "monospace"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            visible: backend.thinkfanWarning.length > 0
            implicitHeight: warningLabel.implicitHeight + 16
            radius: 4
            color: window.warnBg
            border.color: window.warnBorder
            border.width: 1

            Label {
                id: warningLabel
                anchors.fill: parent
                anchors.margins: 8
                text: backend.thinkfanWarning
                wrapMode: Text.WordWrap
                color: window.warnText
                font.bold: true
            }
        }

        Label {
            text: "Fan Mode"
            color: window.textColor
            font.bold: true
            font.pixelSize: 14
        }

        ButtonGroup {
            id: modeGroup
            buttons: [autoRadio, manualRadio]

            function syncFromFanMode() {
                modeGroup.checkedButton = backend.fanMode === "auto" ? autoRadio
                    : backend.fanMode === "manual" ? manualRadio
                    : null
            }

            Component.onCompleted: syncFromFanMode()
        }

        Connections {
            target: backend
            function onFanModeChanged() { modeGroup.syncFromFanMode() }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            RadioButton {
                id: autoRadio
                text: "Auto"
                enabled: backend.readable
                ButtonGroup.group: modeGroup
                palette.windowText: autoRadio.enabled ? window.textColor : window.mutedColor
                onToggled: if (checked) backend.setAuto()
            }

            RadioButton {
                id: manualRadio
                text: "Manual"
                enabled: backend.readable
                ButtonGroup.group: modeGroup
                palette.windowText: manualRadio.enabled ? window.textColor : window.mutedColor
                onToggled: if (checked) backend.setManual(Math.round(manualSlider.value))
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.topMargin: -16
            Layout.bottomMargin: -8
            spacing: 0

            Slider {
                id: manualSlider
                Layout.fillWidth: true
                from: 0
                to: 7
                stepSize: 1
                snapMode: Slider.SnapAlways
                enabled: backend.readable && manualRadio.checked

                property bool syncing: false

                function syncFromBackend() {
                    if (pressed || syncing) {
                        return
                    }
                    syncing = true
                    value = backend.manualValue
                    syncing = false
                }

                Component.onCompleted: syncFromBackend()
                Connections {
                    target: backend
                    function onManualValueChanged() { manualSlider.syncFromBackend() }
                }

                onPressedChanged: {
                    if (!pressed) {
                        backend.setManual(Math.round(value))
                    }
                }
            }

            Label {
                text: Math.round(manualSlider.value)
                color: manualSlider.enabled ? window.textColor : window.mutedColor
                font.family: "monospace"
                Layout.preferredWidth: 24
                horizontalAlignment: Text.AlignRight
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Button {
                text: "Full-speed"
                enabled: backend.readable
                Layout.fillWidth: true
                onClicked: backend.setFullSpeed()
            }

            Button {
                text: "Disengaged"
                enabled: backend.readable
                Layout.fillWidth: true
                palette.buttonText: window.dangerText
                onClicked: {
                    if (window.disengagedAcknowledged) {
                        backend.setDisengaged()
                    } else {
                        disengagedDialog.open()
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: backend.statusText.length > 0
            text: backend.statusText
            wrapMode: Text.WordWrap
            color: backend.statusText.startsWith("Error") || backend.statusText.startsWith("Fan interface unavailable")
                   ? window.errorColor
                   : backend.statusText.startsWith("Fan level left at")
                     ? window.warnText
                     : window.textColor
        }

    }

    DisengagedDialog {
        id: disengagedDialog
        darkMode: backend.darkMode
        dangerColor: window.errorColor
        onConfirmed: {
            window.disengagedAcknowledged = true
            backend.setDisengaged()
        }
    }
}
