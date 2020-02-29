import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, Pango
from CLProg import prog


file_num = 0
func_num = 0


def set_file_num(ind):
        global file_num
        file_num = ind


def set_func_num(ind):
        global func_num
        func_num = ind


class VarData:
    def __init__(self):
        pass


class VarViewer(Gtk.Box):
    def __init__(self, var_data, code_data):
        Gtk.Box.__init__(self)

        self.var_data = var_data
        self.code_data = code_data

        self.var_list_store = Gtk.ListStore(bool, str, int, int)

        self.update_vals("")
        self.create_obj()

    def create_obj(self):
        self.var_list_view = Gtk.TreeView(model=self.var_list_store)

        renderer_debug = Gtk.CellRendererToggle()
        renderer_debug.connect("toggled", self.toggle_var)
        column_toggle = Gtk.TreeViewColumn("", renderer_debug, active=0)
        column_toggle.set_sort_column_id(0)
        self.var_list_view.append_column(column_toggle)

        renderer_name = Gtk.CellRendererText()
        column_name = Gtk.TreeViewColumn("Variables", renderer_name, text=1)
        column_name.set_sort_column_id(1)
        self.var_list_view.append_column(column_name)

        var_index = Gtk.CellRendererText()
        var_index_col = Gtk.TreeViewColumn("", var_index, text=3)
        var_index_col.set_sort_column_id(3)
        # self.var_list_view.append_column(var_index_col)

        self.pack_start(self.var_list_view, True, True, 0)

        self.selected_var = self.var_list_view.get_selection()

    def update_vals(self, selection):

        if selection == "":
            return

        model, row = selection.get_selected()

        set_file_num(model[row][2])
        global file_num

        if self.var_list_store is not None:
            self.var_list_store.clear()
        variables = prog.files[model[row][2]].function_list[0].var_list

        for item in range(len(variables)):
            if type(variables[item]) is tuple:
                break
            self.var_list_store.append((variables[item].isChecked, variables[item].name, int(variables[item].line), item))

    def update_vals_from_func(self, selection):

        global file_num

        if selection == "":
            return

        model, row = selection.get_selected()

        try:
            set_func_num(model[row][3])
        except Exception as e:
            print("Something happened:", e)

        if self.var_list_store is not None:
            self.var_list_store.clear()
        variables = prog.files[file_num].function_list[model[row][3]].var_list

        for item in range(len(variables)):
            if type(variables[item]) is tuple:
                break
            self.var_list_store.append((variables[item].isChecked, variables[item].name, int(variables[item].line), item))

    def toggle_var(self, widget, path):
        global file_num
        global func_num
        self.var_list_store[path][0] = not self.var_list_store[path][0]
        for item in prog.files[file_num].function_list[func_num].var_list:
            if item.name == self.var_list_store[path][1]:
                item.isChecked = not item.isChecked

    def toggle_after_select(self, widget, path):
        if self.var_list_store is not None:
            self.var_list_store.clear()
        variables = prog.files[file_num].function_list[int(path)].var_list

        for item in range(len(variables)):
            if type(variables[item]) is tuple:
                break
            self.var_list_store.append((variables[item].isChecked, variables[item].name, int(variables[item].line), item))
