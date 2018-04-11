#------------------------------
"""Class :py:class:`CMWDBTree` is a QWTree for database-collection tree presentation
======================================================================================

Usage ::
    # Test: python lcls2/psana/psana/graphqt/CMWDBTree.py

    # Import
    from psana.graphqt.CMConfigParameters import

    # See test at the EOF

See:
  - :class:`CMWMain`
  - :class:`CMWDBTree`
  - `on github <https://github.com/slac-lcls/lcls2>`_.

Created on 2017-03-23 by Mikhail Dubrovin
"""
#------------------------------

from psana.graphqt.CMConfigParameters import cp
from psana.graphqt.QWTree import *
import psana.graphqt.CMDBUtils as dbu
#import psana.pscalib.calib.MDBUtils as dbu
from PyQt5.QtCore import pyqtSignal # Qt 

#------------------------------

class CMWDBTree(QWTree) :
    """GUI for database-collection tree 
    """
    db_and_collection_selected = pyqtSignal('QString','QString')

    def __init__(self, parent=None) :

        QWTree.__init__(self, parent)
        self._name = self.__class__.__name__
        cp.cmwdbtree = self
        self.set_selection_mode(cp.cdb_selection_mode.value())
        self.db_and_collection_selected.connect(self.on_db_and_collection_selected)


    def fill_tree_model(self, pattern='') :

        client = dbu.connect_client()

        #pattern = 'cdb_xcs'
        #pattern = 'cspad'
        dbnames = dbu.database_names(client)
        if pattern :
            dbnames = [name for name in dbnames if pattern in name]

        self.clear_model()

        for dbname in dbnames :
            parentItem = self.model.invisibleRootItem()
            #parentItem.setIcon(icon.icon_folder_open)

            itdb = QStandardItem(dbname)
            itdb.setIcon(icon.icon_folder_closed)

            #itdb.setCheckable(True) 
            parentItem.appendRow(itdb)

            db = dbu.database(client, dbname) 

            for col in dbu.collection_names(db) :
                if not col : continue
                itcol = QStandardItem(col)  
                itcol.setIcon(icon.icon_folder_closed)
                itdb.appendRow(itcol)

                #item.setIcon(icon.icon_table)
                #item.setCheckable(True) 
                #print('append item %s' % (item.text()))

#------------------------------

    def on_click(self, index) :
        """Override method from QWTree"""
        item = self.model.itemFromIndex(index)
        itemname = item.text()
        parent = item.parent()
        parname = parent.text() if parent is not None else None
        msg = 'on_click item: %s parent: %s' % (itemname, parname) # index.row()
        logger.info(msg, self._name)
        if parent is not None : self.db_and_collection_selected.emit(parname, itemname)


    def on_db_and_collection_selected(self, dbname, colname) :
        msg = 'on_db_and_collection_selected DB: %s collection: %s' % (dbname, colname)
        logger.info(msg, self._name)
        wdocs = cp.cmwdbdocs
        if wdocs is None : return
        wdocs.show_documents(dbname, colname)

#------------------------------

if __name__ == "__main__" :
    import sys
    from PyQt5.QtWidgets import QApplication
    app = QApplication(sys.argv)
    w = CMWDBTree()
    w.setGeometry(10, 25, 400, 600)
    w.setWindowTitle(w._name)
    w.move(100,50)
    w.show()
    app.exec_()
    del w
    del app

#------------------------------