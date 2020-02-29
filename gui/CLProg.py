import os
import sys

class Variable:
    """
    Most base object. Represents the variables denoted in the input conf file.
    """

    def __init__(self, name, line, id):
        self.name = name
        self.line = line
        self.id = id
        self.isChecked = False


class Function:
    """
    Object that represents function denoted in config file. Is used to be stored in File class
    """

    def __init__(self, name, file_path, line, variable_list):
        """
        Constructor for the Function class
        :param name: Name of Function
        # :param check: Boolean denotes whether function is marked for extra checking
        :param variable_list: List of variables and their memaddrs (tuples)
        """

        self.name = name
        self.line = line
        self.file_path = file_path
        self.var_list = variable_list
        self.isCF = False
        self.isChecked = False

    # def setspecial(self, val):
    #     """
    #     Directly controls the boolean value of a whether a function is checked for extra checking
    #     :param val: Boolean value
    #     :return: void
    #     """
    #
    #     self.isCF = val

    def add_var_list(self, var_list):
        """
        Directly adds variables to the variable list of the function
        :param var_list: list of variables imported
        :return: void
        """
        tuple(var_list)
        print(var_list)
        self.var_list += 0

    def set_all_false(self):
        for item in self.var_list:
            item.isChecked = False

    def set_all_true(self):
        for item in self.var_list:
            item.isChecked = True


class File:
    """
    Object that represents file attached to a function denoted in config file. Is used to be stored in CLProg class
    """
    def __init__(self, filepath):
        self.function_list = []
        self.filepath = filepath
        self.line_map = {}
        self.isChecked = False
        self.generate_line_map()

    def generate_line_map(self):
        for item in range(len(self.function_list)):
            # print(type(self.function_list[item].line), " : ", type(self.function_list[item].name))
            self.line_map.update({self.function_list[item].line: self.function_list[item].name})

    def add_function(self, new_func):
        self.function_list.append(new_func)
        self.generate_line_map()


class CLProg:
    """
    A collection of Function objects stored for easy access and interface with the GUI
    """

    def __init__(self):
        """
        Constructor for the function class
        """

        self.files = []
        self.paths = []
        self.functions = []
        self.locatelist = []
        self.filelist = []
        self.possible_lines = set()
        self.import_conf()

    def add_file(self, new_file):
        """
        Adds a new function to the CLProg
        :param new_file:
        :return:
        """

        self.files.append(new_file)

    def import_conf(self):
        """
        Imports config file, creates the functions necessary, and stores them in the CLProg object
        :return: void
        """
        tmp = 1
        name = ""
        filepath = ""
        line_num = ""
        var_list = ""
        found_dup = False
        # try:
        dirName = sys.argv[1]
        hlsDebugVariableListFile = dirName + "hlsDebugVariableList.txt"
        print (hlsDebugVariableListFile)
        file = open(hlsDebugVariableListFile, 'r')
        for line in file:
            # print("Line: ", line)
            func_params = []
            if line.strip():
                if tmp == 1:
                    tmp += 1
                    function_call = line.rstrip()
                    for field in function_call.split(':'):
                        func_params.append(field)
                    name = func_params[0]
                    filepath = func_params[1]
                    line_num = func_params[2]
                elif tmp == 2:
                    tmp = 1
                    var_list = []
                    for var in line.rstrip().split(','):
                        params = []
                        # print(var)
                        for field in var.split(':'):
                            params.append(field)
                        # self.possible_lines.add(str(params[1]))
                        var_list.append(Variable(str(params[0])+":"+str(params[1]), params[1], params[2]))
                    # self.possible_lines.add(str(line_num))

                    if filepath not in self.paths:
                        self.paths.append(filepath)
                        CLProg.add_file(self, File(filepath))

                    for file_object in self.files:
                        if filepath == file_object.filepath:
                            file_object.add_function(Function(name, filepath, line_num, var_list))
        file.close()
        # except IndexError as e:
        #     print("Expected config file for argument. Err:2")
        #     exit(2)

    def export_conf(self):
        dirName = sys.argv[1]
        hlsDebugSelectedVarFile = dirName + "hlsDebugSelectedVar.txt"
        f = open(hlsDebugSelectedVarFile, "w+")
        output = ""
        for file in range(len(self.files)):
            for i in range(len(self.files[file].function_list)):
                # output += str(self.files[file].function_list[i].name + ':' + self.files[file].filepath + '\n')
                # output += str("CF=" + str(self.functions[i].isCF) + '\n')
                for var in self.files[file].function_list[i].var_list:
                    if var.isChecked:
                        output += str(var.id + ' ')
                # output += '\n\n'
        f.write(output)
        f.close()

    def locate_function(self, func_name):
        # THIS IGNORES CPP FILES THAT ARE USED WITH CMAKE
        root_dir = sys.argv[1] #'~/rose/exm_opencl_vector_add_x64_linux/vector_add/device/'
        for folder, dirs, files in os.walk(root_dir):
            for file in files:
                if file.endswith('.c'):
                    full_path = os.path.join(folder, file)
                    with open(full_path, 'r') as f:
                        print(full_path)
                        for line in f:
                            # print(line)
                            if func_name in line:
                                self.locatelist.append(full_path)
                                break

    def open_c(self, ind):
        file = open(self.paths[ind], 'r')
        for line in file:
            self.filelist.append(line.rstrip())
        file.close()

prog = CLProg()
