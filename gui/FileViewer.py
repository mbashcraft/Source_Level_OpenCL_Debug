import gi
import os
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, Pango
from CLProg import prog

class FileData:
    def __init__(self):
        self.prog = prog
        self.file_index = -1

    def get_file_index(self):
        return self.file_index

    def set_file_index(self, index):
        self.file_index = index


class FileViewer(Gtk.Box):
    def __init__(self, file_data, func_data, code_data):
        Gtk.Box.__init__(self)

        self.file_data = file_data
        self.func_data = func_data
        self.code_data = code_data

        self.file_list_store = Gtk.ListStore(bool, str, int)

        self.update_vals()
        self.create_obj()

    def create_obj(self):
        file_list_view = Gtk.TreeView(self.file_list_store)
        file_bool = Gtk.CellRendererToggle()
        file_bool.connect("toggled", self.file_on_cell_toggled)
        file_bool_col = Gtk.TreeViewColumn("", file_bool, active=0)
        file_bool_col.set_sort_column_id(0)
        file_list_view.append_column(file_bool_col)

        file_name = Gtk.CellRendererText()
        file_name_col = Gtk.TreeViewColumn("File", file_name, text=1)
        file_name_col.set_sort_column_id(1)
        file_list_view.append_column(file_name_col)

        file_index = Gtk.CellRendererText()
        file_index_col = Gtk.TreeViewColumn("", file_index, text=2)
        file_index_col.set_sort_column_id(2)
        # file_list_view.append_column(file_index_col)

        self.file_select = file_list_view.get_selection()
        # self.file_select.connect("changed", self.file_selected)
        # self.file_select.connect("changed", self.code_viewer_on_cell_toggle)
        # self.file_select.connect("changed", self.update_code_viewer)

        self.pack_start(file_list_view, True, True, 0)

    def update_vals(self):
        for item in range(len(self.file_data.prog.files)):
            self.file_list_store.append(
                (self.file_data.prog.files[item].isChecked, os.path.basename(os.path.normpath(self.file_data.prog.files[item].filepath)), item))

    def file_on_cell_toggled(self, widget, path):

        self.file_list_store[path][0] = not self.file_list_store[path][0]

    def file_selected(self, selection):
        model, row = selection.get_selected()

        # for item in range(len(self.file_data.prog.files)):
            # if model[row][1] == os.path.basename((os.path.normpath(self.file_data.prog.files[item].filepath))):
                # self.file_data.set_file_index(item)
                # break
