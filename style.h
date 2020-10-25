// Written by Adrian Musceac YO8RZZ , started March 2016.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef STYLE_H
#define STYLE_H

#include <QString>

static const QString night_stylesheet =
        "QTabWidget::pane { border: 1px solid #71717c;}"
        "QTabWidget::tab-bar {left: 2px;}"
        "QTabBar::tab { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #4a5053, stop: 1.0 #505a62);; color: #ffffd3; padding: 8px; padding-left: 20px;padding-right: 20px;"
            " border: 1px solid #000000; border-style:outset; border-top-left-radius: 4px;border-top-right-radius: 4px;} "
        "QTabBar::tab:selected { background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #40484a, stop: 1.0 #4a5053); color: #ffffd3;} "
        "QTabBar::tab:!selected { margin-top: 5px;}"
        "QTabBar::tab:hover:!selected {background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #505a62, stop: 0.4 #00374f, stop: 0.5 #00374f, stop: 1.0 #505a62);}"
        "QTabBar::tab:first:selected {margin-left: 0px;}"
        "QTabBar::tab:last:selected {margin-right: 0px;}"
        "QWidget#memoryControlsFrame {background: none;}"
        "QWidget#memoriesFrame {background: none;}"
        "QWidget#memoriesTableWidget {background: none;}"
        "QWidget, QTabWidget {background-color:#505a62; color:#ffffd3;}"
        "QFrame {background-color:#505a62; color:#ffffd3;border-radius:6px;border:0px solid darkgrey;}"
        "QFrame#controlsFrame, QFrame#ledFrame, QFrame#mumbleControlsFrame, QFrame#shiftEditFrame {border-radius:6px;border:1px solid darkgrey;}"
        "QLabel {border:1px solid #505050;border-radius:4px;background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #4a5053, stop: 1.0 #505a62); color: #ffffd3;}"
        "QPushButton#buttonTransmit {font: 18pt \"Sans Serif\";color: #bbbbbb; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #770000, stop: 0.3 #990000, stop: 0.7 #990000, stop: 1.0 #770000);border: 1px solid #CC0000; border-style: outset;}"
        "QPushButton#buttonTransmit:checked {font: 18pt \"Sans Serif\";color: #ffffff; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #880000, stop: 0.3 #CC0000, stop: 0.7 #CF0000, stop: 1.0 #AA0000);border: 1px solid #330000; border-style: inset;}"
        "QPushButton {background-color:qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #5f6669, stop: 0.3 #5a697d, stop: 0.5 #505a62, stop: 1.0 #4f5659); color:#ffffd3;"
            "border-radius: 4px;border:1px solid #747474;border-style:outset;margin:2px;padding:5px;}"
        "QPushButton:hover {background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #505a62, stop: 0.4 #00374f, stop: 0.5 #00374f, stop: 1.0 #505a62); color:#ffffd3;border:1px solid #002c86;}"
        "QPushButton:checked, QPushButton:hover:pressed {background-color:#4a5053; color:#ffffd3;border-style:inset;}"
        "QPushButton:checked:hover {background-color: #4a5053; border-style:inset;}"
        "QCheckBox:hover {background-color:#555555; color:#ffffd3;}"
        "QCheckBox:checked {color:#fffb6f;}"
        "QHeaderView::section {background-color: rgba(0, 40, 102, 25);}"
        "QTableView {selection-background-color: rgba(20, 135, 206, 170);color:#ffffff}"
        "QDial {background-color:#5a5a5a; color:#ffffd3;}"
        "QDial:hover {background-color:#002c86; color:#ffffd3}"
        "QComboBox {border: 1px solid #1487ce; border-style: outset;border-radius: 6px; color: #f0f077; padding: 3px 6px 3px 6px; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #0a619c, stop: 1.0 #1487ce);}"
        "QComboBox QAbstractItemView {border:1px solid grey;selection-background-color: #70c2ed; color: #f0f077; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #0a619c, stop: 1.0 #1487ce);}"
        "QComboBox::drop-down {border: 0px solid gray;image:url(\":/res/go-down-arrow.png\");subcontrol-origin: padding;subcontrol-position: right;margin-right:4px;}"
        "QTreeWidgetItem {background-color:#001e5a; color:#ffffd3;}"
        "QTreeWidgetItem:hover {background-color:#002c86; color:#ffffd3}"
        "QLineEdit, QTextEdit, QPlainTextEdit {background-color:#a3a3a3; color:#000000;border: 1px solid #dddddd; border-style:inset; border-radius:4px;}"
        "QLineEdit:hover, QTextEdit:hover, QPlainTextEdit:hover {border: 2px solid #55aaff; border-radius:4px;}"
        "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {border: 2px solid #2277ff;border-radius:4px;}"
        "QSlider::sub-page:vertical {border: 1px solid #bbb;"
            "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #0a619c, stop: 1.0 #1487ce);"
            "border: 1px solid #777;border-style:outset;height: 5px;border-radius: 3px;}"
        "QSlider::sub-page:horizontal {border: 1px solid #bbb;"
            "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #0a619c, stop: 1.0 #1487ce);"
            "border: 1px solid #777;border-style:outset;height: 5px;border-radius: 3px;}"
        "QSlider::add-page {"
            "background-color: #67696e;border: 1px solid #777;border-style:inset;height: 5px;border-radius: 3px;}"
        "QSlider::handle {"
            "background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
            " stop:0 #eee, stop:1 #ccc);"
            "border: 1px solid #777;width: 16px;border-radius: 4px;}"
        "QSlider::handle:hover {"
            "background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
            " stop:0 #fff, stop:1 #ddd);"
            "border: 1px solid #444;border-radius: 4px;}"
        "QSlider::sub-page:disabled {background-color: #bbb;border-color: #999;}"
        "QSlider::add-page:disabled {background-color: #eee;border-color: #999;}"
        "QSlider::handle:disabled {background-color: #eee;border: 1px solid #aaa;border-radius: 4px;};";


static const QString day_stylesheet =
        "QTabWidget::pane { border: 1px solid #B2B7BB;}"
        "QTabWidget::tab-bar {left: 2px;}"
        "QTabBar::tab { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #d0dae4, stop: 0.3 #dfdfdf, stop: 0.7 #c7c7c7, stop: 1.0 #d0dae4); color: #000000; padding: 8px; padding-left: 20px;padding-right: 20px;"
            "border: 1px solid #aaaaaa; border-style:outset; border-top-left-radius: 5px;border-top-right-radius: 5px;} "
        "QTabBar::tab:selected {background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #fafafa, stop: 0.4 #f4f4f4, stop: 0.5 #e7e7e7, stop: 1.0 #fafafa); color: #000000; } "
        "QTabBar::tab:!selected { margin-top: 5px;}"
        "QTabBar::tab:hover:!selected {background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #fafafa, stop: 0.4 #f4f4f4, stop: 0.5 #e7e7e7, stop: 1.0 #fafafa);}"
        "QTabBar::tab:first:selected {margin-left: 0;}"
        "QTabBar::tab:last:selected {margin-right: 0;}"
        "QWidget#memoryControlsFrame {background: none}"
        "QWidget#memoriesFrame {background: none;}"
        "QWidget#memoriesTableWidget {background: none}"
        "QWidget, QTabWidget {background-color:#d0dae4; color:#000000}"
        "QFrame {background-color:#d0dae4; color:#000000;border-radius:6px;border:0px solid darkgrey;}"
        "QFrame#controlsFrame, QFrame#ledFrame, QFrame#mumbleControlsFrame, QFrame#shiftEditFrame {border-radius:6px;border:1px solid darkgrey;}"
        "QLabel {border:1px solid darkgrey;border-radius:4px;background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #cad0d9, stop: 1.0 #d0dae4); color: #232323;}"
        "QPushButton#buttonTransmit {font: 18pt \"Sans Serif\";color: #bbbbbb; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #770000, stop: 0.3 #990000, stop: 0.7 #990000, stop: 1.0 #770000);border: 1px solid #CC0000; border-style: outset;}"
        "QPushButton#buttonTransmit:checked {font: 18pt \"Sans Serif\";color: #ffffff; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #880000, stop: 0.3 #CC0000, stop: 0.7 #CF0000, stop: 1.0 #AA0000);border: 1px solid #330000; border-style: inset;}"
        "QPushButton {margin:2px;background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #d0dae4, stop: 0.4 #dfdfdf, stop: 0.5 #c7c7c7, stop: 1.0 #d0dae4);padding:5px; border: 1px solid gray; border-style: outset; border-radius:4px;}"
        "QPushButton:checked, QPushButton:hover:pressed {background-color: #a3d4f1; border-style:inset;}"
        "QPushButton:checked:hover {background-color: #a3d4f1; border-style:inset;}"
        "QPushButton:hover {margin:2px;background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #fafafa, stop: 0.4 #f4f4f4, stop: 0.5 #e7e7e7, stop: 1.0 #fafafa); border: 1px solid gray; border-style: outset; border-radius:4px;}"
        "QCheckBox:hover {background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #fafafa, stop: 0.4 #f4f4f4, stop: 0.5 #e7e7e7, stop: 1.0 #fafafa);}"
        "QCheckBox:checked {color:#585800}"
        "QHeaderView::section {background-color: rgba(0, 40, 102, 25);}"
        "QTableView {selection-background-color: rgba(20, 135, 206, 170);color:#ffffff}"
        "QDial {background-color:#9a9a9a; color:#ffffd3;border:0px solid black;}"
        "QDial:hover {background-color:#002c86; color:#ffffd3}"
        "QComboBox {border: 1px solid #1487ce; border-style: outset;border-radius: 6px; color: #f0f077; padding: 3px 6px 3px 6px; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #0a619c, stop: 1.0 #1487ce);}"
        "QComboBox QAbstractItemView {border:1px solid grey;selection-background-color: #70c2ed; color: #f0f077; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #0a619c, stop: 1.0 #1487ce);}"
        "QComboBox::drop-down {border: 0px solid gray;image:url(\":/res/go-down-arrow.png\");subcontrol-origin: padding;subcontrol-position: right;margin-right:4px;}"
        "QTreeWidgetItem {background-color:#505a62; color:#ffffd3}"
        "QTreeWidget {background-color:#505a62; color:#ffffd3}"
        "QTreeWidgetItem:hover {background-color:#a3d4f1; color:#ffffd3}"
        "QLineEdit, QTextEdit, QPlainTextEdit {background-color:#f0f0f0; color:#000000;border: 1px solid #dddddd; border-style:inset; border-radius:4px;}"
        "QLineEdit:hover, QTextEdit:hover, QPlainTextEdit:hover {background-color:#f0f0f0; color:#000000;border: 2px solid #55aaff; border-radius:4px;}"
        "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {border: 2px solid #2277ff;border-radius:4px;}"
        "QSlider::sub-page:vertical {border: 1px solid #bbb;"
            "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #0a619c, stop: 1.0 #1487ce);"
            "border: 1px solid #777;border-style:outset;height: 5px;border-radius: 3px;}"
        "QSlider::sub-page:horizontal {border: 1px solid #bbb;"
            "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #0a619c, stop: 1.0 #1487ce);"
            "border: 1px solid #777;border-style:outset;height: 5px;border-radius: 3px;}"
        "QSlider::add-page {border: 1px solid #bbb;"
            "background-color: #ffffd3;border: 1px solid #777;border-style:inset;height: 5px;border-radius: 3px;}"
        "QSlider::handle {"
            "background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
            " stop:0 #eee, stop:1 #ccc);"
            "border: 1px solid #777;width: 6px;height:6px;border-radius: 4px;}"
        "QSlider::handle:hover {"
            "background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
            " stop:0 #fff, stop:1 #ddd);"
            "border: 1px solid #444;border-radius: 4px;}"
        "QSlider::sub-page:disabled {background-color: #bbb;border-color: #999;}"
        "QSlider::add-page:disabled {background-color: #eee;border-color: #999;}"
        "QSlider::handle:disabled {background-color: #eee;border: 1px solid #aaa;border-radius: 4px;};";

#endif // STYLE_H
