/*
 *   Copyright 2012 Daniel Nicoletti <dantti12@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import Qt 4.7
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.qtextracomponents 0.1

Item {
    id: jobItem
    width: jobItem.ListView.view.width
    property bool expanded: jobItem.ListView.view.currentIndex == index
    height: expanded ? 90 : 30
    Behavior on height { PropertyAnimation {} }

    PlasmaCore.FrameSvgItem {
        id: padding
        imagePath: "widgets/viewitem"
        prefix: "hover"
        opacity: 0
        Behavior on opacity { PropertyAnimation {} }
        anchors.fill: parent
        
        Component.onCompleted: {
            // Try to avoid calling jobsView directly
            jobsView.highlight.connect(highlightJobs)
        }
        function highlightJobs(printer) {
            if (printer == jobPrinter) {
                padding.opacity = 0.7;
            } else {
                padding.opacity = 0;
            }
        }
    }
    
    MouseArea {
        id: container
        anchors {
            fill: parent
            topMargin: padding.margins.top
            leftMargin: padding.margins.left
            rightMargin: padding.margins.right
            bottomMargin: padding.margins.bottom
        }
        hoverEnabled: true
        onEntered: {
            padding.opacity = 0.7;
        }
        onClicked: {
            if (jobItem.ListView.view.currentIndex == index) {
                jobItem.ListView.view.currentIndex = -1;
            } else {
                jobItem.ListView.view.currentIndex = index;
            }
        }
        onExited: {
            padding.opacity = 0;
        }
        
        Column {
            spacing: 8
            anchors.fill: parent
            Row {
                id: jobRow
                spacing: 4
                width: parent.width
                height: jobNameLabel.paintedHeight

                QIconItem {
                    id: jobIcon
                    width: 16
                    height: 16
                    icon: QIcon(jobIconName)
                }
                PlasmaComponents.Label {
                    // 12 = 3 * spacing
                    id: jobNameLabel
                    width: parent.width - pagesLabel.width - jobIcon.width - 12
                    height: paintedHeight
                    elide: Text.ElideRight
                    text: jobName
                }
                PlasmaComponents.Label {
                    id: pagesLabel
                    horizontalAlignment: "AlignRight"
                    height: paintedHeight
                    width: paintedWidth
                    text: jobPages
                    visible: jobPages != 0
                    font.pointSize: theme.smallestFont.pointSize
                    color: "#99"+(theme.textColor.toString().substr(1))
                }
            }
            
            Item {
                id: actionRow
                opacity: expanded ? 1 : 0
                width: parent.width
                height: ownerLabel.height + sizeLabel.height + createdLabel.height
                Behavior on opacity { PropertyAnimation {} }
                Row {
                    spacing: 4
                    anchors.fill: parent
                    Column {
                        spacing: 2
                        PlasmaComponents.ToolButton {
                            id: cancelButton
                            flat: true
                            iconSource: "dialog-cancel"
                            text: "Cancel Job"
                            visible: jobCancelEnabled
                            onClicked: {
                                service = printersSource.serviceForSource(DataEngineSource);
                                operation = service.operationDescription("cancelJob");
                                operation.PrinterName = jobPrinter;
                                service.startOperationCall(operation);
                            }
                        }
                        PlasmaComponents.ToolButton {
                            id: holdButton
                            flat: true
                            iconSource: "document-open-recent"
                            text: jobHoldEnabled ? "Hold Job" : "Release Job"
                            visible: jobCancelEnabled
                            onClicked: {
                                service = printersSource.serviceForSource(DataEngineSource);
                                operation = service.operationDescription(jobHoldEnabled ? "holdJob" : "releaseJob");
                                operation.PrinterName = jobPrinter;
                                service.startOperationCall(operation);
                            }
                        }
                    }
                    Column {
                        spacing: 2
                        Row {
                            PlasmaComponents.Label {
                                id: ownerLabel
                                horizontalAlignment: "AlignRight"
                                height: paintedHeight
                                width: paintedWidth
                                text: i18n("Owner: ")
                                font.pointSize: theme.smallestFont.pointSize
                                color: "#99"+(theme.textColor.toString().substr(1))
                            }
                            PlasmaComponents.Label {
                                horizontalAlignment: "AlignLeft"
                                height: paintedHeight
                                width: paintedWidth
                                text: jobOwner
                                font.pointSize: theme.smallestFont.pointSize
                                color: "#99"+(theme.textColor.toString().substr(1))
                            }
                        }
                        Row {
                            PlasmaComponents.Label {
                                id: sizeLabel
                                horizontalAlignment: "AlignRight"
                                height: paintedHeight
                                width: paintedWidth
                                text: i18n("Size: ")
                                font.pointSize: theme.smallestFont.pointSize
                                color: "#99"+(theme.textColor.toString().substr(1))
                            }
                            PlasmaComponents.Label {
                                horizontalAlignment: "AlignLeft"
                                height: paintedHeight
                                width: paintedWidth
                                text: jobSize
                                font.pointSize: theme.smallestFont.pointSize
                                color: "#99"+(theme.textColor.toString().substr(1))
                            }
                        }
                        Row {
                            PlasmaComponents.Label {
                                id: createdLabel
                                horizontalAlignment: "AlignRight"
                                height: paintedHeight
                                width: paintedWidth
                                text: i18n("Created: ")
                                font.pointSize: theme.smallestFont.pointSize
                                color: "#99"+(theme.textColor.toString().substr(1))
                            }
                            PlasmaComponents.Label {
                                horizontalAlignment: "AlignLeft"
                                height: paintedHeight
                                width: paintedWidth
                                text: jobCreatedAt
                                font.pointSize: theme.smallestFont.pointSize
                                color: "#99"+(theme.textColor.toString().substr(1))
                            }
                        }
                    }
                }
            }
        }
    }
}