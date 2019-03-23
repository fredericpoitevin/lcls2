#--------------------
"""
:py:class:`CGWConfigEditor` - widget for configuration editor
============================================================================================

Usage::

    # Import
    from psdaq.control_gui.CGWConfigEditor import CGWConfigEditor

    # Methods - see test

See:
    - :py:class:`CGWConfigEditor`
    - `lcls2 on github <https://github.com/slac-lcls/lcls2/psdaq/psdaq/control_gui>`_.

This software was developed for the LCLS2 project.
If you use all or part of it, please give an appropriate acknowledgment.

Created on 2019-03-08 by Mikhail Dubrovin
"""
#--------------------

import logging
logger = logging.getLogger(__name__)
from PyQt5.QtWidgets import QTabBar
from PyQt5.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QTextEdit, QPushButton, QFileDialog, QComboBox
# , QWidget, QLabel, QLineEdit, QGroupBox
#from PyQt5.QtCore import pyqtSignal #, Qt, QRectF, QPointF, QTimer

from psdaq.control_gui.CGWConfigEditorTree import CGWConfigEditorTree
from psdaq.control_gui.CGWConfigEditorText import CGWConfigEditorText
#from psdaq.control_gui.CGWConfigEditorList import CGWConfigEditorList

from psdaq.control_gui.Utils import save_textfile, path_to_test_data
from psdaq.control_gui.CGJsonUtils import load_json_from_file, str_json, json_from_str

#--------------------

EDITOR_TYPES = ('Text','Tree') # ,'List'
char_expand  = u' \u25BC' # down-head triangle
char_shrink  = u' \u25B2' # solid up-head triangle

#--------------------

class CGWConfigEditor(QWidget) :
    """
    """
    def __init__(self, parent=None, parent_ctrl=None):

        #QGroupBox.__init__(self, 'Partition', parent)
        QWidget.__init__(self, parent)
        self.parent_ctrl = parent_ctrl

        # I/O files
        self.ifname_json = '%s/json2xtc_test.json' % path_to_test_data() # input file
        self.ofname_json = './test.json' # output file

        self.dictj = None
        self.load_dict() # fills self.dictj

        self.but_load = QPushButton('Load')
        self.but_save = QPushButton('Save')
        self.but_expn = QPushButton('Expand %s'%char_expand)
        self.box_type = QComboBox(self)

        self.box_type.addItems(EDITOR_TYPES)
        self.box_type.setCurrentIndex(1)
        self.wedi = CGWConfigEditorTree(parent_ctrl=self, dictj=self.dictj)

        self.hbox = QHBoxLayout() 
        self.hbox.addWidget(self.but_load)
        self.hbox.addWidget(self.but_save)
        self.hbox.addWidget(self.but_expn)
        self.hbox.addStretch(1)
        self.hbox.addWidget(self.box_type)

        self.vbox = QVBoxLayout() 
        self.vbox.addLayout(self.hbox)
        self.vbox.addWidget(self.wedi)
        self.setLayout(self.vbox)

        self.set_tool_tips()
        self.set_style()

        self.but_load.clicked.connect(self.on_but_load)
        self.but_save.clicked.connect(self.on_but_save)
        self.but_expn.clicked.connect(self.on_but_expn)
        self.box_type.currentIndexChanged[int].connect(self.on_box_type)

#--------------------

    def set_tool_tips(self) :
        self.setToolTip('Configuration editor GUI')
        self.box_type.setToolTip('Select editor type')
        self.but_expn.setToolTip('Expand/Collapse tree-like content')
        self.but_save.setToolTip('Save content in file')
        self.but_load.setToolTip('Load content from file')

#--------------------

    def set_style(self) :
        from psdaq.control_gui.Styles import style
        #self.grb_read_nodes.setStyleSheet(style.qgrbox_title)
        #self.grb_proc_nodes.setStyleSheet(style.qgrbox_title)
        #self.grb_bld       .setStyleSheet(style.qgrbox_title)
        #self.grb_camera_ioc.setStyleSheet(style.qgrbox_title)

        self.setWindowTitle('Configuration Editor')
        self.setMinimumSize(400,800)
        self.layout().setContentsMargins(0,0,0,0)
  
        #self.but_save.setStyleSheet(style.styleButton) 
        #self.but_save.setVisible(True)
 
        #self.setMinimumWidth(300)
        #self.edi.setMinimumWidth(210)
        #self.setFixedHeight(34) # 50 if self.show_frame else 34)
        #if not self.show_frame : 
        #self.layout().setContentsMargins(0,0,0,0)

        #style = "background-color: rgb(255, 255, 220); color: rgb(0, 0, 0);" # Yellowish
        #style = "background-color: rgb(100, 240, 200); color: rgb(0, 0, 0);" # Greenish
        #style = "background-color: rgb(255, 200, 220); color: rgb(0, 0, 0);" # Pinkish
        #style = "background-color: rgb(240, 240, 100); color: rgb(0, 0, 0);" # YellowBkg
        #self.setStyleSheet(style)

        #self.setFixedSize(750,270)
        #self.setMaximumWidth(800)
 
#--------------------

    def load_dict(self) :
        ifname = self.ifname_json
        logger.info('CGWConfigEditor: load json from %s' % ifname)
        self.dictj = dj = load_json_from_file(ifname)
        sj = str_json(dj)
        logger.info('CGWConfigEditor: dict of json as str:\n%s' % sj)

#--------------------
 
    def on_but_load(self):
        logger.debug('on_but_load')
        if self.select_ifname() : 
           self.load_dict()
           self.wedi.set_content(self.dictj)

#--------------------
 
    def select_ifname(self):
        logger.info('select_ifname %s' % self.ifname_json)
        path, ftype = QFileDialog.getOpenFileName(self,
                      caption   = 'Select the file to load json',
                      directory = self.ifname_json,
                      filter    = 'Text files(*.json *.cfg *.txt *.text *.dat *.data)\nAll files (*)'
                      )
        if path == '' :
            logger.info('Loading is cancelled')
            return False
        self.ifname_json = path
        logger.info('Input file: %s' % path)
        return True

#--------------------
 
    def on_but_save(self):
        logger.debug('on_but_save')
        if self.select_ofname() : 
           self.save_dict_in_file()

#--------------------
 
    def select_ofname(self):
        logger.info('select_ofname %s' % self.ofname_json)
        path, ftype = QFileDialog.getSaveFileName(self,
                      caption   = 'Select the file to save json',
                      directory = self.ofname_json,
                      filter    = 'Text files (*.json *.cfg *.txt *.text *.dat *.data)\nAll files (*)'
                      )
        if path == '' :
            logger.info('Saving is cancelled')
            return False
        self.ofname_json = path
        logger.info('Output file: %s' % path)
        return True

#--------------------
 
    def save_dict_in_file(self):
        logger.info('save_dict_in_file %s' % self.ofname_json)
        dj = self.wedi.get_content()
        sj = str_json(dj)
        save_textfile(sj, self.ofname_json, mode='w', verb=True)

#--------------------
 
    def on_box_type(self, ind) :
        type = EDITOR_TYPES[ind]
        logger.info('CGWConfigEditor set editor type %s' % type)

        if self.wedi is not None :
           self.wedi.close()
           del self.wedi

        self.wedi = self.editor_widget_selector(type)
        self.vbox.addWidget(self.wedi)
        self.wedi.setVisible(True)

        #self.show_documents(self.dbname, self.colname)

#--------------------

    def editor_widget_selector(self, edi_type):
        """Factory method for selection of the editor widget.
        """
        logger.info('Set document browser in mode %s' % edi_type)

        self.but_expn.setVisible(edi_type=='Tree')
        
        kwargs = {'parent':None, 'parent_ctrl':self, 'dictj':self.dictj}

        if   edi_type == EDITOR_TYPES[0] : return CGWConfigEditorText(**kwargs)
        elif edi_type == EDITOR_TYPES[1] : return CGWConfigEditorTree(**kwargs)
        #elif edi_type == EDITOR_TYPES[2] : return CGWConfigEditorList(**kwargs)
        else :
            logger.warning('Unknown editor type "%s"' % edi_type)
            return QTextEdit(edi_type)

#--------------------
 
    def on_but_expn(self):
        #logger.debug('on_but_expn')
        if self.but_expn.text()[:6] == 'Expand' :
           self.wedi.process_expand()
           self.but_expn.setText('Collapse %s'%char_shrink)
        else :
           self.wedi.process_collapse()
           self.but_expn.setText('Expand %s'%char_expand)

#--------------------

#    def closeEvent(self, e):
#        logger.debug('closeEvent')
#        #self.parent_ctrl.w_display = None

#--------------------

if __name__ == "__main__" :

    logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s', datefmt='%H:%M:%S', level=logging.DEBUG)
    import sys
    from PyQt5.QtWidgets import QApplication
    app = QApplication(sys.argv)
    w = CGWConfigEditor(None)
    #w.connect_path_is_changed_to_recipient(w.test_signal_reception)
    w.show()
    app.exec_()

#--------------------