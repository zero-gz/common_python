# -*- coding: utf-8 -*-

import sys
reload(sys)
sys.setdefaultencoding("utf-8")

from PyQt4 import QtGui, QtCore

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Button(QtGui.QPushButton):
    def __init__(self, parent):
        super(Button, self).__init__(parent)
        self.setAcceptDrops(True)
        #self.setDragDropMode(QAbstractItemView.InternalMove)

    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
        else:
            super(Button, self).dragEnterEvent(event)

    def dragMoveEvent(self, event):
        super(Button, self).dragMoveEvent(event)

    def dropEvent(self, event):
        if event.mimeData().hasUrls():

            #遍历输出拖动进来的所有文件路径
            for url in event.mimeData().urls():
                file_name1 = url.toLocalFile()
                file_name2 = str(url.toLocalFile()).decode('UTF-8').encode('GBK')
                with open(file_name1) as fp:
                    pass
                with open(file_name2) as fp2:
                    pass

            event.acceptProposedAction()
        else:
            super(Button,self).dropEvent(event)

class MyWindow(QtGui.QWidget):
    def __init__(self):
        super(MyWindow,self).__init__()
        self.setGeometry(100,100,300,400)
        self.setWindowTitle("Filenames")

        self.btn = Button(self)
        self.btn.setGeometry(QtCore.QRect(90, 90, 61, 51))
        self.btn.setText("Change Me!")
        layout = QtGui.QVBoxLayout(self)
        layout.addWidget(self.btn)

        self.setLayout(layout)

if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    window = MyWindow()
    window.show()
    sys.exit(app.exec_())