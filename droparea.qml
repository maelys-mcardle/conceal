import QtQuick 1.1
import QtQuick 1.0

Rectangle {
	id: background

	width:600
	height:480
	color: "#1e1414"

	Text {
		id: dropText
		text: "Drop Files Here"
		font.pointSize: 18
		color: "white"
		anchors.verticalCenter: parent.verticalCenter
		anchors.horizontalCenter: parent.horizontalCenter
	}

	Text {
		id: subtitle
		text: "Putting all the files together"
		color: "white"
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.top: plaintext.bottom
	}

	Image {
		id: plaintext
		anchors.verticalCenter: parent.verticalCenter
		anchors.horizontalCenter: parent.horizontalCenter
		opacity: 0
		source: "qrc:/images/plaintext.png"
	}

	Image {
		id: archive
		anchors.verticalCenter: parent.verticalCenter
		anchors.horizontalCenter: parent.horizontalCenter
		opacity: 1
		source: "qrc:/images/archive.png"
	}

	Image {
		id: ciphertext
		anchors.verticalCenter: parent.verticalCenter
		anchors.horizontalCenter: parent.horizontalCenter
		opacity: 0
		source: "qrc:/images/ciphertext.png"
	}


 states: [
	 State {
		 name: "Processing"

		 PropertyChanges {
			 target: dropText
			 visible: false
		 }

		 PropertyChanges {
			 target: ciphertext
			 opacity: 1
		 }
	 }
 ]

}
