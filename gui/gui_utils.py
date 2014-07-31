import os

import gtk  
import gtk.glade  
import gtk.gdk
import gobject
import glib

from numpy import *

app_base_dir = os.path.dirname(os.path.realpath(__file__))

def imagefile2pixbuf(filename, size):  
    from PIL import Image
    import StringIO
    im = Image.open(filename)
    im.thumbnail(size, Image.ANTIALIAS)
    file1 = StringIO.StringIO()  
    im.save(file1, "ppm")  
    contents = file1.getvalue()  
    file1.close()  
    loader = gtk.gdk.PixbufLoader("pnm")  
    loader.write(contents, len(contents))  
    pixbuf = loader.get_pixbuf()  
    pixbuf = pixbuf.add_alpha(True, 0xFF, 0xFF, 0xFF)
    loader.close()  
    return pixbuf  


def set_tab_header(name, close_callback, bold):  
    filename = os.path.join(app_base_dir, 'resources/tab.ui')
    builder = gtk.Builder() 
    builder.add_from_file(filename)

    tab = builder.get_object("tab")
    label = builder.get_object("label")
    if bold:
        label.set_text("<b>%s</b>" %name)
        label.set_use_markup(True)
    else:
        label.set_text(name)

    button = builder.get_object("button_close")   
    button.connect("button-press-event", close_callback)
    return tab


def setup_sourceview(language, font_name):
    import gtksourceview2
    import pango
    sv = gtksourceview2.View(gtksourceview2.Buffer())
    sv.set_show_line_numbers(False)
    sv.set_show_line_marks(False)
    sv.set_auto_indent(True)
    sv.set_tab_width(4)
    sv.set_insert_spaces_instead_of_tabs(True)
    sv.set_show_right_margin(True)

    lm = gtksourceview2.LanguageManager()
    l = lm.get_language(language)
    
    b = sv.get_buffer()
    b.set_max_undo_levels(100)
    b.set_language(l)
    b.set_highlight_syntax(True)
    #sv.connect("key-press-event", self.on_textview2_key_press_event)

    if font_name is not None:
        sv.get_pango_context()
        font_description = pango.FontDescription(font_name)
        sv.modify_font(font_description)
    return sv


def my_get_visible_range(sw):
    adj = sw.get_vadjustment()
    rowsize = 22.
    start = max(int(adj.get_value() / rowsize - 1), 0)
    length = int(adj.get_page_size() / rowsize) + 1
    end = start + length
    return start, end


def is_row_visible(model, iter, view):
    visible_range = view.get_visible_range()
    if visible_range == None:
        return False
    begin, end = visible_range
    path = model.get_path(iter)
    if path[0] not in range(begin[0], end[0] + 1):
        return False
    return True


def get_current_nb_text(notebook):
    n = notebook.get_current_page()
    page = notebook.get_nth_page(n)
    return notebook.get_tab_label_text(page)  

def get_active_cb_text(combobox):
    '''
        returns the selected value of a combobox
    '''
    model = combobox.get_model()
    active = combobox.get_active()
    if active < 0:
        return None
    return model[active][0] 

def set_active_cb_text(combobox, text):
    model = combobox.get_model()
    for i in xrange(len(model)):
        a = model[i][0] 
        if a == text:
            combobox.set_active(i)
            return i
    return -1       



def show_file_dialog(dialog, name=""):
    if len(name):
        dialog.set_current_name(name)
    response = dialog.run()
    if response != gtk.RESPONSE_OK: #cancel button
        dialog.hide()    
        return None
    file = dialog.get_filename()   
    dialog.hide()  
    gtk.main_iteration_do()
    return file  

class builder_ui(object):
    def __init__(self, builder=None, filename=None):
        if builder is None:
            if filename:
                builder = gtk.Builder() 
                if os.path.isfile(filename):
                    builder.add_from_file(filename)
                else:
                    base = os.path.dirname(__file__)
                    fn = os.path.join(base, filename)
                    if os.path.isfile(fn):
                        builder.add_from_file(fn)
                    else:
                        fn = os.path.join(base, "resources", filename)
                        if os.path.isfile(fn):
                            builder.add_from_file(fn)
                        else:
                            raise Exception("file %r not found!" % filename)
                builder.connect_signals(self)
            else:
                raise Exception("either need builder or ui-filename!")
        self.builder = builder
        self._show_groups = {}

    def __getattr__(self, name):
        widget = self.builder.get_object(name)
        if widget is None:
            raise AttributeError(name)
        setattr(self, name, widget)
        return widget
        
    def new_show_group(self, group_name):
        class show_group(object):
            def __init__(self, parent, name):
                self.parent = parent
                self.name = name
                self.members = {}
                self.visible = None
            def add(self, member):
                if type(member) in (str, unicode):
                    key, value = member, getattr(self.parent, member)
                else:
                    key, value = member.get_name(), member
                self.members[key] = value
                if len(self.members) == 1:
                    self.show(key)
            def show(self, member):
                if type(member) in (str, unicode):
                    key = member
                else:
                    key = member.get_name()
                to_show = self.members[key]
                for other in self.members.values():
                    if other == to_show:
                        continue
                    other.hide()
                to_show.show()
                self.visible = member
            def update(self):
                self.show(self.visible)
        setattr(self, group_name, show_group(self, group_name))
        
        
