import QtQuick
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Window is the root element for a desktop application window
Window {
    width: 400
    height: 300
    visible: true
    title: qsTr("PiCAN Stopwatch")
    color: "#2b2b2b" // Dark background

    // ColumnLayout stacks items vertically
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20

        // The Time Display
        Text {
            Layout.alignment: Qt.AlignHCenter
            // Binding: This text automatically updates whenever
            // backend.elapsedTimeString emits its NOTIFY signal.
            text: backend.elapsedTimeString
            font.pixelSize: 48
            font.family: "Courier New" // Monospaced font looks better for numbers
            color: "#ffffff"
        }

        // RowLayout stacks items horizontally for buttons
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 20

            // Button 1: Pause / Resume
            Button {
                // The text changes based on the C++ isRunning state
                text: backend.isRunning ? "Pause" : "Resume"
                font.pixelSize: 16
                onClicked: {
                    // Call the C++ function
                    backend.togglePauseResume()
                }
            }

            // Button 2: Stop / Reset
            Button {
                // Logic for label based on prompt requirements:
                // If running -> "Stop"
                // If not running AND has recorded time -> "Reset"
                // Otherwise (stopped at 00:00) -> "Reset" (could also disable it here)
                text: backend.isRunning ? "Stop" : "Reset"

                // Optional: Disable reset button if time is already 00:00:000
                // enabled: backend.isRunning || backend.hasTime

                font.pixelSize: 16
                onClicked: {
                    backend.stopReset()
                }
            }
        }
    }
}