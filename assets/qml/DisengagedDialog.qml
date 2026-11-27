import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Dialog {
    id: dialog
    modal: true
    title: "Confirm Disengaged Mode"
    anchors.centerIn: Overlay.overlay
    width: Math.min(Overlay.overlay ? Overlay.overlay.width - 48 : 460, 460)
    standardButtons: Dialog.NoButton
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    required property bool darkMode
    required property color dangerColor
    signal confirmed()

    Material.theme: dialog.darkMode ? Material.Dark : Material.Light
    Material.primary: dialog.darkMode ? Material.color(Material.Grey, Material.Shade300)
                                      : Material.color(Material.Grey, Material.Shade800)
    Material.accent: Material.primary

    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, dialog.darkMode ? 0.52 : 0.32)
    }

    function resetAcknowledgement() {
        confirmCheck.checked = false
    }

    onAboutToShow: resetAcknowledgement()
    onClosed: resetAcknowledgement()

    ColumnLayout {
        width: parent ? parent.width : implicitWidth
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: "Disengaged mode disables EC closed-loop speed regulation and drives the fan open-loop at maximum power."
            wrapMode: Text.WordWrap
        }

        CheckBox {
            id: confirmCheck
            Layout.fillWidth: true
            text: "I understand"
            Material.accent: dialog.dangerColor
            Material.foreground: dialog.dangerColor
        }

        RowLayout {
            spacing: 8
            Layout.fillWidth: true

            Button {
                text: "Cancel"
                Layout.fillWidth: true
                onClicked: dialog.reject()
            }

            Button {
                text: "Disengage"
                enabled: confirmCheck.checked
                Layout.fillWidth: true
                Material.foreground: dialog.dangerColor
                onClicked: {
                    dialog.close()
                    dialog.confirmed()
                }
            }
        }
    }
}
