import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, Pango
from CLProg import prog
from FileViewer import FileData



class CodeData:
    def __init__(self):
        self.prog = prog
        # self.update()
        self.data_list = ["derp", "flerp"]
        # self.possible_lines = []

    def update(self, selection):
        if selection == "":
            return
        self.data_list.clear()
        model, row = selection.get_selected()
        self.prog.open_c(model[row][2])
        self.data_list = self.prog.filelist

class CodeViewer(Gtk.Box):
    def __init__(self, code_data, file_data):
        Gtk.Box.__init__(self)

        self.file_data = file_data
        self.code_data = code_data

        # self.data_list = data_list
        # self.possible_lines = possible_lines

        # Initial Values
        self.linecount = 1
        self.background = Gdk.RGBA(0, 0, 0, 0)
        self.fontdesc_code = Pango.FontDescription("mono 10")
        self.fontdesc_line = Pango.FontDescription("mono 8")

        self.list_store = Gtk.ListStore(int, str, Gdk.RGBA)
        self.list_view = Gtk.TreeView(self.list_store)
        self.line_renderer = Gtk.CellRendererText()
        self.code_renderer = Gtk.CellRendererText()
        self.line_column_title = Gtk.TreeViewColumn("Line", self.line_renderer, text=0)
        self.code_column_title = Gtk.TreeViewColumn("", self.code_renderer, text=1)
        self.code_column_title.add_attribute(self.code_renderer, "background-rgba", 2)
        self.line_renderer.set_property('foreground', 'white')
        self.line_renderer.set_property('font-desc', self.fontdesc_line)
        self.line_renderer.set_alignment(True, False)
        self.line_renderer.set_property('background-rgba', Gdk.RGBA(0, 0, 0, .6))
        self.code_renderer.set_property('font-desc', self.fontdesc_code)

        self.pack_start(self.list_view, True, True, 0)

        self.update_vals("")

    def update_vals(self, selection):
        if selection == "":
            return

        model, row = selection.get_selected()

        if self.list_store is not None:
            self.list_store.clear()

        self.linecount = 1

        for item in range(len(self.code_data.data_list)):
            if item % 2 == 0:
                self.background = Gdk.RGBA(0, 0, 0, .1)
            else:
                self.background = Gdk.RGBA(0, 0, 0, 0)
            # if str(self.linecount) in self.possible_lines:
                # self.background = Gdk.RGBA(0, 1, 0, 0.2)
            self.list_store.append((self.linecount, self.code_data.data_list[item]) + (self.background,))
            self.linecount += 1

        self.list_view.append_column(self.line_column_title)

        self.list_view.append_column(self.code_column_title)

        self.show_all()
