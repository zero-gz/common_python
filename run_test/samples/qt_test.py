#!/usr/bin/python
# -*- coding: utf-8 -*-

# colordialog.py

import sys
from PyQt4 import QtGui
from PyQt4 import QtCore


class Example(QtGui.QWidget):

    def __init__(self):
        super(Example, self).__init__()

        self.initUI()

    def initUI(self):

        color = QtGui.QColor(0, 0, 0)

        self.button = QtGui.QPushButton('Dialog', self)
        self.button.setFocusPolicy(QtCore.Qt.NoFocus)
        self.button.move(20, 20)

        self.connect(self.button, QtCore.SIGNAL('clicked()'),
            self.showDialog)
        self.setFocus()

        self.widget = QtGui.QWidget(self)
        self.widget.setStyleSheet("QWidget { background-color: %s }"
            % color.name())
        self.widget.setGeometry(130, 22, 100, 100)

        self.label = QtGui.QLabel('Knowledge only matters', self)
        self.label.move(20, 200)

        self.setWindowTitle('ColorDialog')
        self.setGeometry(300, 300, 800, 600)

        self.label = QtGui.QLabel(self)
        edit = QtGui.QLineEdit(self)

        edit.move(60, 100)
        self.label.move(60, 40)

        self.connect(edit, QtCore.SIGNAL('textChanged(QString)'),
                     self.onChanged)

        self.label1 = QtGui.QLabel("Ubuntu", self)

        combo = QtGui.QComboBox(self)
        combo.addItem("Ubuntu")
        combo.addItem("Mandriva")
        combo.addItem("Fedora")
        combo.addItem("Red Hat")
        combo.addItem("Gentoo")

        combo.move(50, 50)
        self.label1.move(50, 150)

        self.connect(combo, QtCore.SIGNAL('activated(QString)'),
                     self.onActivated)

    def onActivated(self, text):
        self.label1.setText(text)
        self.label1.adjustSize()

    def onChanged(self, text):
        self.label.setText(text)
        self.label.adjustSize()


    def showDialog(self):
        font, ok = QtGui.QFontDialog.getFont()
        if ok:
            self.label.setFont(font)

        # col = QtGui.QColorDialog.getColor()
        #
        # if col.isValid():
        #     self.widget.setStyleSheet("QWidget { background-color: %s }"
        #         % col.name())


if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    ex = Example()
    ex.show()
    app.exec_()