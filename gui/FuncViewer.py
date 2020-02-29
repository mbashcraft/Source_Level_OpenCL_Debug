import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, Pango
from CLProg import prog

file_num = 0


def set_file_num(ind):
        global file_num
        file_num = ind


class FuncData:
    def __init__(self):
        pass


class FuncViewer(Gtk.Box):
    def __init__(self, func_data, var_data, code_data):
        Gtk.Box.__init__(self)

        self.func_data = func_data
        self.var_data = var_data
        self.code_data = code_data

        self.function_list_store = Gtk.ListStore(bool, str, int, int)

        self.update_vals("")
        self.create_obj()

    def create_obj(self):
        self.function_list_view = Gtk.TreeView(self.function_list_store)
        self.func_renderer_debug = Gtk.CellRendererToggle()
        self.func_renderer_debug.connect("toggled", self.toggle_func)
        func_column_toggle = Gtk.TreeViewColumn("", self.func_renderer_debug, active=0)
        func_column_toggle.set_sort_column_id(0)
        self.function_list_view.append_column(func_column_toggle)

        func_renderer_name = Gtk.CellRendererText()
        func_column_name = Gtk.TreeViewColumn("Function", func_renderer_name, text=1)
        func_column_name.set_sort_column_id(1)
        self.function_list_view.append_column(func_column_name)

        func_index = Gtk.CellRendererText()
        func_index_col = Gtk.TreeViewColumn("", func_index, text=3)
        func_index_col.set_sort_column_id(3)
        # self.function_list_view.append_column(func_index_col)

        self.selected_row = self.function_list_view.get_selection()
        # self.selected_row.connect("changed", self.item_selected)
        # self.selected_row.connect("changed", self.create_var_tree)
        # self.selected_row.connect("changed", self.snap_to_func)

        self.pack_start(self.function_list_view, True, True, 0)

    def update_vals(self, selection):
        if selection == "":
            return

        if self.function_list_store is not None:
            self.function_list_store.clear()

        model, row = selection.get_selected()

        index = model[row][2]
        set_file_num(index)

        for item in range(len(prog.files[index].function_list)):
            self.function_list_store.append((prog.files[index].function_list[item].isChecked,
                                             prog.files[index].function_list[item].name,
                                             int(prog.files[index].function_list[item].line), item))

        print("Lines", prog.files[index].line_map)
        # all_func = Gtk.CheckButton()
        # all_func.connect("toggled", self.func_on_cell_toggled)

    def toggle_func(self, widget, path):
        global file_num
        self.function_list_store[path][0] = not self.function_list_store[path][0]
        for item in prog.files[file_num].function_list:
            if item.name == self.function_list_store[path][1]:
                if item.isChecked:
                    item.isChecked = False
                    item.set_all_false()
                else:
                    item.isChecked = True
                    item.set_all_true()
