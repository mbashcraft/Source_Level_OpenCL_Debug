import gi
from CodeViewer import CodeViewer, CodeData
from VarViewer import VarViewer, VarData
from FuncViewer import FuncViewer, FuncData
from FileViewer import FileViewer, FileData

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, Pango, GObject
from CLProg import prog


class Main(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="Homogeneous Debugger")
        self.set_default_size(1000, 900)
        self.set_border_width(15)
        self.set_position(Gtk.WindowPosition.CENTER)
        self.set_gravity(Gdk.Gravity.CENTER)

        # HEADER BAR FOR MAIN WINDOW
        header_bar = Gtk.HeaderBar()
        header_bar.set_show_close_button(True)
        header_bar.props.title = "Debugger"
        self.set_titlebar(header_bar)
        select_button = Gtk.Button("Confirm")
        select_button.connect("clicked", self.update_vals)
        header_bar.pack_end(select_button)

        # Creating data classes that will be shared by views
        self.file_data = FileData()
        self.func_data = FuncData()
        self.var_data = VarData()
        self.code_data = CodeData()

        # Creating the specialized Gtk boxes
        self.code_viewer = CodeViewer(self.code_data, self.file_data)
        self.var_viewer = VarViewer(self.var_data, self.code_data)
        self.func_viewer = FuncViewer(self.func_data, self.var_data, self.code_data)
        self.file_viewer = FileViewer(self.file_data, self.func_data, self.code_data)

        # Declares scroll windows for boxes
        vscroll_code = Gtk.ScrolledWindow()
        vscroll_variables = Gtk.ScrolledWindow()
        vscroll_function = Gtk.ScrolledWindow()
        vscroll_file = Gtk.ScrolledWindow()

        # Set scroll policy
        vscroll_code.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        vscroll_variables.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        vscroll_function.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        vscroll_file.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)

        # Packs objects in scrollable spaces
        vscroll_code.add(self.code_viewer)
        vscroll_variables.add(self.var_viewer)
        vscroll_function.add(self.func_viewer)
        vscroll_file.add(self.file_viewer)

        # Box to organize filters
        left_box = Gtk.HBox(spacing=20)
        left_box.pack_start(vscroll_file, True, True, 0)
        left_box.pack_start(vscroll_function, True, True, 0)
        left_box.pack_start(vscroll_variables, True, True, 0)

        # FileViewer selection
        self.file_viewer.file_select.connect("changed", self.var_viewer.update_vals)
        self.file_viewer.file_select.connect("changed", self.func_viewer.update_vals)
        self.file_viewer.file_select.connect("changed", self.code_data.update)
        self.file_viewer.file_select.connect("changed", self.code_viewer.update_vals)

        # FuncViewer Selection
        self.func_viewer.selected_row.connect("changed", self.var_viewer.update_vals_from_func)
        self.func_viewer.func_renderer_debug.connect_after("toggled", self.var_viewer.toggle_after_select)

        # Paned Box to organize settings and code
        main_pane = Gtk.HPaned()
        main_pane.add1(left_box)
        main_pane.add2(vscroll_code)

        self.add(main_pane)

    def update_vals(self, widget):
        prog.export_conf()
        exit(0)


def main():
    window = Main()
    window.connect("delete-event", Gtk.main_quit)
    window.show_all()
    Gtk.main()


if __name__ == "__main__":
    main()
