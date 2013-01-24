import QtQuick 1.1

Rectangle {
	id: master
	state: "Default"
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
		opacity: 1
	}

	Text {
		id: subtitle
		objectName: "progressText"
		text: "Putting all the files together"
		color: "white"
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.top: plaintext.bottom
		opacity: 0
	}

	Image {
		id: plaintext
		objectName: "plaintextImage"
		anchors.verticalCenter: parent.verticalCenter
		anchors.horizontalCenter: parent.horizontalCenter
		opacity: 0
		source: "qrc:/images/plaintext.png"
	}

	Image {
		id: archive
		objectName: "archiveImage"
		anchors.verticalCenter: parent.verticalCenter
		anchors.horizontalCenter: parent.horizontalCenter
		opacity: 0
		source: "qrc:/images/archive.png"
	}

	Image {
		id: ciphertext
		objectName: "ciphertextImage"
		anchors.verticalCenter: parent.verticalCenter
		anchors.horizontalCenter: parent.horizontalCenter
		opacity: 0
		source: "qrc:/images/ciphertext.png"
	}

	MouseArea {
		id: infoMouseArea
		anchors.left: parent.left
		anchors.top: parent.top
		width: parent.width/3
		height: parent.height
		hoverEnabled: true
		onClicked: { infoBackground.state = "ShowInfo"; }
	}

	Rectangle {
		id: infoBackground
		rotation: 270
		anchors.centerIn: parent
		width: parent.height
		height: parent.width
		opacity: 0
		gradient: Gradient {
			GradientStop { position: 0; color: "#d0000090"; }
			GradientStop { position: 0.5; color: "#d01e1414"; }
			GradientStop { position: 1; color: "#d01e1414"; }
		}

		Item {
			id: deRotateInfo
			rotation: 90
			width: parent.height - 20
			height: parent.width - 20
			anchors.centerIn: parent

			Text {
				id: infoTitle
				anchors.left: parent.left
				anchors.top: parent.top
				text: "Conceal"
				font.pixelSize: 24
				color: "white"
				opacity: 0
			}

			Text {
				id: infoTooltip
				text: "click for information"
				font.italic: true
				anchors.top: parent.top
				anchors.left: parent.left
				color: "white"
				style: Text.Raised
				styleColor: "gray"
				opacity: 0
			}

			Text {
				id: infoText
				anchors.left: infoTitle.left
				anchors.top: infoTitle.bottom
				width: master.width - 30
				text: "\nConceal is a program to encrypt and decrypt files. " +
				"It's intent is to bring privacy to those would want it, " +
				"but might be intimidated by more comprehensive products.\n\n " +
				"This software uses 128-bit RIJNDAEL encryption, implemented by the " +
				"MCRYPT library. The included icons were designed by the Oxygen " +
				"Project (oxygen-icons.org) and are distributed under an LGPLv3 " +
				"license. This program was developed using Qt.\n\n" +
				"Released under the terms of the GPLv3."
				color: "white"
				style: Text.Raised
				styleColor: "gray"
				wrapMode: Text.WordWrap
				opacity: 0
			}

			Text {
				id: infoLicenseLink
				anchors.left: parent.left
				anchors.bottom: parent.bottom
				font.underline: true
				text: "Full License"
				color: "yellow"
				opacity: 0

				MouseArea {
					id: infoLicenseMouseArea
					anchors.left: parent.left
					anchors.top: parent.top
					width: parent.width
					height: parent.height
					onClicked: { mainWindow.showLicense(); }
				}
			}
		}

		states: [
			State {
				name: "HintInfo"
				when: (infoMouseArea.containsMouse)
				PropertyChanges { target: infoBackground; opacity: 1; }
				PropertyChanges { target: infoTooltip; opacity: 1; }
			},

			State {
				name: "HideInfo"
				when: (!infoMouseArea.containsMouse)
				PropertyChanges { target: infoText; opacity: 0; }
				PropertyChanges { target: infoTitle; opacity: 0; }
				PropertyChanges { target: infoBackground; opacity: 0; }
				PropertyChanges { target: infoTooltip; opacity: 0; }
				PropertyChanges { target: infoLicenseLink; opacity: 0; }
			},

			State {
				name: "ShowInfo"
				PropertyChanges { target: infoText; opacity: 1; }
				PropertyChanges { target: infoTitle; opacity: 1; }
				PropertyChanges { target: infoBackground; opacity: 1; }
				PropertyChanges { target: infoTooltip; opacity: 0; }
				PropertyChanges { target: infoLicenseLink; opacity: 1; }
			}
		]

		transitions: Transition {
			NumberAnimation { properties: "opacity"; easing.type: Easing.InOutQuad }
		}
	}

	MouseArea {
		id: cancelMouseArea
		enabled: false
		anchors.right: parent.right
		anchors.top: parent.top
		width: parent.width/3
		height: parent.height
		hoverEnabled: true
		onClicked: { mainWindow.cancelCrypto(); }
	}

	Rectangle {
		id: cancelBackground
		rotation: 270
		anchors.centerIn: parent
		width: parent.height
		height: parent.width
		opacity: 0
		gradient: Gradient {
			GradientStop { position: 1; color: "#d0900000"; }
			GradientStop { position: 0.5; color: "#d01e1414"; }
			GradientStop { position: 0; color: "#d01e1414"; }
		}

		Item {
			id: deRotateCancel
			rotation: 90
			width: parent.height - 20
			height: parent.width - 20
			anchors.centerIn: parent

			Text {
				id: cancelText
				anchors.right: parent.right
				anchors.verticalCenter: parent.verticalCenter
				text: "Cancel"
				font.pixelSize: 24
				color: "white"
				opacity: 0
			}
		}

		states: [
			State {
				name: "ShowCancel"
				when: (cancelMouseArea.containsMouse)
				PropertyChanges { target: cancelBackground; opacity: 1; }
				PropertyChanges { target: cancelText; opacity: 1; }
			},

			State {
				name: "HideCancel"
				when: (!cancelMouseArea.containsMouse)
				PropertyChanges { target: cancelBackground; opacity: 0; }
				PropertyChanges { target: cancelText; opacity: 0; }
			}
		]

		transitions: Transition {
			NumberAnimation { properties: "opacity"; easing.type: Easing.InOutQuad }
		}

	}

	states: [
		State {
			name: "Default"
			PropertyChanges { target: infoMouseArea; enabled: true; }
			PropertyChanges { target: cancelMouseArea; enabled: false; }
			PropertyChanges { target: cancelBackground; opacity: 0; }
			PropertyChanges { target: cancelText; opacity: 0; }
			PropertyChanges { target: dropText; opacity: 1; }
			PropertyChanges { target: subtitle; opacity: 0; }
			PropertyChanges { target: ciphertext; opacity: 0; }
			PropertyChanges { target: archive; opacity: 0; }
			PropertyChanges { target: plaintext; opacity: 0; }
		},

		State {
			name: "Processing"
			PropertyChanges { target: infoMouseArea; enabled: false; }
			PropertyChanges { target: cancelMouseArea; enabled: true; }
			PropertyChanges { target: dropText; opacity: 0; }
			PropertyChanges { target: subtitle; opacity: 1; }
			PropertyChanges { target: ciphertext; opacity: 1; }
			PropertyChanges { target: archive; opacity: 1; }
			PropertyChanges { target: plaintext; opacity: 1; }
		}
	]

	transitions: Transition {
		NumberAnimation { properties: "opacity"; easing.type: Easing.InOutQuad }
	}
}
