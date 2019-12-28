import sys
from PyQt5.QtWidgets import *
from PyQt5.QtCore import *
from Data.geoui import Ui_Form
from PyQt5.QtCore import *
from PyQt5.QtGui import *


class mwindow(QWidget, Ui_Form):
    def __init__(self):
        super(mwindow, self).__init__()
        self.setupUi(self)
        pixmap = QPixmap("./Data/trapezoid.png")
        self.label_image.setPixmap(pixmap)
        pixmap = QPixmap("./Data/triangle.png")
        self.label.setPixmap(pixmap)
        self.label_para_1.setText("角A")
        self.label_para_2.setText("角B")
        self.label_para_3.setText("角C")
        self.label_para_4.setText("角D")
        self.label_para_5.setText("边a")
        self.label_para_6.setText("边b")
        self.label_para_7.setText("边c")
        self.label_para_8.setText("边d")
        self.label_para_9.setText("高AE")

        self.label_2_para_1.setText("角A")
        self.label_2_para_2.setText("角B")
        self.label_2_para_3.setText("角C")
        self.label_2_para_4.setText("角D")
        self.label_2_para_5.setText("边a")
        self.label_2_para_6.setText("边b")
        self.label_2_para_7.setText("高H")
        self.listWidget.currentRowChanged.connect(
            self.stackedWidget.setCurrentIndex)
        self.pushbtn_calc.pressed.connect(self.pushbtn_calc_pressed)
        self.pushbtn2_calc.pressed.connect(self.pushbtn2_calc_pressed)

    def pushbtn_calc_pressed(self):
        self.pushbtn_calc.setText("PRESSED")

    def pushbtn2_calc_pressed(self):
        self.pushbtn2_calc.setText("PRESSED")



if __name__ == '__main__':
    app = QApplication(sys.argv)
    w = mwindow()
    w.show()
    sys.exit(app.exec_())
