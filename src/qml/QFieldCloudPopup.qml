import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import org.qfield 1.0
import Theme 1.0

Popup {
  id: popup
  padding: 0

  Page {
    anchors.fill: parent

    header: PageHeader {
      title: qsTr('QFieldCloud')

      showApplyButton: false
      showCancelButton: cloudProjectsModel.currentProjectStatus === QFieldCloudProjectsModel.Idle
      showBusyIndicator: cloudConnection.status === QFieldCloudConnection.Connecting ||
                         cloudProjectsModel.currentProjectStatus === QFieldCloudProjectsModel.Uploading ||
                         cloudProjectsModel.currentProjectStatus === QFieldCloudProjectsModel.Downloading

      onCancel: {
        popup.close()
      }
    }

    ColumnLayout {
      visible: cloudProjectsModel.currentProjectId && cloudConnection.status !== QFieldCloudConnection.LoggedIn
      id: connectionSettings
      anchors.fill: parent
      spacing: 2

      QFieldCloudLogin {
        id: qfieldCloudLogin
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.margins: 10
      }

      Item {
          Layout.fillHeight: true
          height: 15
      }
    }

    ColumnLayout {
      visible: !cloudProjectsModel.currentProjectId
      anchors.fill: parent
      anchors.margins: 20
      anchors.topMargin: 50
      spacing: 2

      Text {
        Layout.fillWidth: true
        font: Theme.defaultFont
        text: '<style>a, a:hover, a:visited { color:' + Theme.mainColor + '; }></style>' +
              qsTr('The current project is not stored on QFieldCloud.<br><br>') +
              qsTr('Storing projects on QFieldCloud offers seamless synchornization, offline editing, and team management.<br><br>') +
              ' <a href="https://qfield.cloud/">' + qsTr( 'Learn more about QFieldCloud' ) + '</a>.'
        textFormat: Text.RichText
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter

        onLinkActivated: Qt.openUrlExternally(link)
      }

      Item {
          Layout.fillHeight: true
          height: 15
      }
    }

    ScrollView {
      visible: cloudProjectsModel.currentProjectId && cloudConnection.status === QFieldCloudConnection.LoggedIn
      padding: 0
      ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
      ScrollBar.vertical.policy: ScrollBar.AsNeeded
      contentWidth: mainGrid.width
      contentHeight: mainGrid.height
      anchors.fill: parent
      clip: true

      GridLayout {
        id: mainGrid
        width: parent.parent.width
        columns: 1
        columnSpacing: 2
        rowSpacing: 2

        RowLayout {
          Text {
              Layout.fillWidth: true
              Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
              id: welcomeText
              padding: 10
              text: switch(cloudConnection.status) {
                    case 0: qsTr( 'Disconnected from the cloud.' ); break;
                    case 1: qsTr( 'Connecting to the cloud.' ); break;
                    case 2: qsTr( 'Greetings %1.' ).arg( cloudConnection.username ); break;
                  }
              wrapMode: Text.WordWrap
              font: Theme.tipFont
          }

          Rectangle {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.margins: 10
            width: 48
            height: 48

            Image {
              anchors.fill: parent
              id: cloudAvatar
              source: 'qrc:/images/qfieldcloud-logo.png'
              width: 48
              height: 48
              sourceSize.width: 48 * screen.devicePixelRatio
              sourceSize.height: 48 * screen.devicePixelRatio
              fillMode: Image.PreserveAspectFit
            }
          }
        }

        Text {
          id: statusText
          visible: cloudProjectsModel.currentProjectStatus === QFieldCloudProjectsModel.Downloading ||
                   cloudProjectsModel.currentProjectStatus === QFieldCloudProjectsModel.Uploading
          font: Theme.defaultFont
          text: switch(cloudProjectsModel.currentProjectStatus ) {
                  case QFieldCloudProjectsModel.Downloading: qsTr('Downloading…'); break;
                  case QFieldCloudProjectsModel.Uploading: qsTr('Uploading…'); break;
                  default: '';
                }
          wrapMode: Text.WordWrap
          horizontalAlignment: Text.AlignHCenter
          Layout.fillWidth: true
        }

        Text {
          id: transferErrorText
          visible: false
          font: Theme.defaultFont
          text: ''
          color: Theme.darkRed
          wrapMode: Text.WordWrap
          horizontalAlignment: Text.AlignHCenter
          Layout.fillWidth: true

          Connections {
            target: cloudProjectsModel

            function onSyncFinished(projectId, hasError, errorString) {
              transferErrorText.visible = hasError && cloudProjectsModel.currentProjectStatus === QFieldCloudProjectsModel.Idle;

              if (transferErrorText.visible)
                transferErrorText.text = errorString
            }

            function onDataChanged() {
              transferErrorText.visible = cloudProjectsModel.currentProjectStatus === QFieldCloudProjectsModel.Idle;
            }
          }
        }

        GridLayout {
          Layout.margins: 10
          id: mainInnerGrid
          width: parent.width
          visible: cloudConnection.status === QFieldCloudConnection.LoggedIn &&
                   cloudProjectsModel.currentProjectStatus === QFieldCloudProjectsModel.Idle
          columns: 1
          columnSpacing: parent.columnSpacing
          rowSpacing: parent.rowSpacing

          Text {
            id: changesText
            font: Theme.tipFont
            color: Theme.gray
            text: cloudProjectsModel.currentProjectChangesCount === 0
                  ? qsTr('There are no local changes.')
                  : cloudProjectsModel.currentProjectChangesCount === 1
                    ? qsTr('There is a single local change.')
                    : qsTr('There are %1 local changes.').arg( cloudProjectsModel.currentProjectChangesCount )
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.bottomMargin: 20
            Layout.fillWidth: true
          }

          QfButton {
            id: syncButton
            Layout.fillWidth: true
            font: Theme.defaultFont
            text: qsTr('Synchronize')
            enabled: cloudProjectsModel.canCommitCurrentProject || cloudProjectsModel.canSyncCurrentProject

            onClicked: uploadProject(true)
          }

          Text {
            id: syncText
            font: Theme.tipFont
            color: Theme.gray
            text: qsTr('Synchronize the whole project with all modified features and download the freshly updated project with all the applied changes from QFieldCloud.')
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.bottomMargin: 20
            Layout.fillWidth: true
          }

          QfButton {
            id: pushButton
            Layout.fillWidth: true
            font: Theme.defaultFont
            text: qsTr('Push changes')
            enabled: cloudProjectsModel.canCommitCurrentProject

            onClicked: uploadProject(false)
          }

          Text {
            id: pushText
            font: Theme.tipFont
            color: Theme.gray
            text: qsTr('Save internet bandwidth by only pushing the local features and pictures to the cloud, without updating the whole project.')
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.bottomMargin: 20
            Layout.fillWidth: true
          }

          QfButton {
            id: discardButton
            Layout.fillWidth: true
            font: Theme.defaultFont
            bgcolor: Theme.darkRed
            text: qsTr('Discard local changes')
            enabled: cloudProjectsModel.canCommitCurrentProject

            onClicked: {
              discardDialog.open();
            }
          }

          Text {
            id: discardText
            font: Theme.tipFont
            color: Theme.gray
            text: qsTr('Revert all modified features in the local cloud layers. You cannot restore those changes.')
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.bottomMargin: 10
            Layout.fillWidth: true
          }
        }
      }
    }
  }

  Connections {
    target: cloudConnection

    function onStatusChanged() {
      if (cloudConnection.status !== QFieldCloudConnection.LoggedIn) {
        visible = false
        displayToast(qsTr('Not logged in'))
      }
    }
  }


  Dialog {
    id: discardDialog
    parent: mainWindow.contentItem

    property int selectedCount: 0
    property bool isDeleted: false

    visible: false
    modal: true

    x: ( mainWindow.width - width ) / 2
    y: ( mainWindow.height - height ) / 2

    title: qsTr( "Discard local changes" )
    Label {
      width: parent.width
      wrapMode: Text.WordWrap
      text: qsTr( "Should local changes be discarded?" )
    }

    standardButtons: Dialog.Ok | Dialog.Cancel

    onAccepted: {
      discardLocalChangesFromCurrentProject();
    }
    onRejected: {
      visible = false
      popup.focus = true;
    }
  }

  function show() {
    visible = !visible

    if ( cloudProjectsModel.currentProjectId && cloudConnection.hasToken && cloudConnection.status === QFieldCloudConnection.Disconnected )
      cloudConnection.login();

    if ( cloudConnection.status === QFieldCloudConnection.Connectiong )
      displayToast(qsTr('Connecting cloud'))
  }

  function uploadProject(shouldDownloadUpdates) {
    if (cloudProjectsModel.canCommitCurrentProject || cloudProjectsModel.canSyncCurrentProject) {
      cloudProjectsModel.uploadProject(cloudProjectsModel.currentProjectId, shouldDownloadUpdates)
      return
    }
  }

  function discardLocalChangesFromCurrentProject() {
    if (cloudProjectsModel.canCommitCurrentProject || cloudProjectsModel.canSyncCurrentProject) {
      if ( cloudProjectsModel.discardLocalChangesFromCurrentProject(cloudProjectsModel.currentProjectId) )
        displayToast(qsTr('Local changes discarded'))
      else
        displayToast(qsTr('Failed to discard changes'))

      return
    }

    displayToast(qsTr('No changes to discard'))
  }
}