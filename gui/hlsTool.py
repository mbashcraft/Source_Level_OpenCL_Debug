import re
import sys
import os
import subprocess

rose_home = os.environ["ROSE_HOME"]
#print(rose_home)

clFileName = sys.argv[1]
cFileName = re.sub("\.cl", ".c", clFileName)
subprocess.call(['rm', cFileName])
split_path = os.path.split(cFileName)
#print split_path[0]
#print split_path[1]

clFile = open(clFileName, "r")
contents = clFile.read()
clFile.close()
new_contents = contents
new_contents = re.sub('__kernel', r'\n/*__kernel*/', new_contents)
new_contents = re.sub('__global', r'\n/*__global*/', new_contents)
new_contents = re.sub('__local', r'\n/*__local*/', new_contents)
new_contents = re.sub('__constant', r'\n/*__constant*/', new_contents)
new_contents = re.sub('__private', r'\n/*__private*/', new_contents)
new_contents = re.sub('__contents', r'\n/*__contents*/', new_contents)
new_contents = re.sub('__channel', r'\n/*__channel*/', new_contents)
new_contents = re.sub('__read_only', r'\n/*__read_only*/', new_contents)
new_contents = re.sub('__write_only', r'\n/*__write_only*/', new_contents)
new_contents = re.sub(' pipe ', r'\n/*pipe*/ ', new_contents)
cFile = open(cFileName, "w")
cFile.write(new_contents)
cFile.close()

defaultTranslatorDir = rose_home + '/build/exampleTranslators/defaultTranslator/'
subprocesscall = defaultTranslatorDir + 'defaultTranslator'
print(subprocesscall)
subprocess.call([subprocesscall, cFileName])


newRoseFile = 'rose_' + split_path[1]
subprocess.call(['mv', newRoseFile, cFileName])

roseCFileName = cFileName
roseCFile = open(roseCFileName, "r")
contents = roseCFile.read()
new_contents = contents
new_contents = re.sub(r'/\*__kernel\*/', r'__kernel', new_contents)
new_contents = re.sub(r'/\*__global\*/', r'__global', new_contents)
new_contents = re.sub(r'/\*__local\*/', r'__local', new_contents)
new_contents = re.sub(r'/\*__constant\*/', r'__constant', new_contents)
new_contents = re.sub(r'/\*__private\*/', r'__private', new_contents)
new_contents = re.sub(r'/\*__contents\*/', r'__contents', new_contents)
new_contents = re.sub(r'/\*__channel\*/', r'__channel', new_contents)
new_contents = re.sub(r'/\*__read_only\*/', r'__read_only', new_contents)
new_contents = re.sub(r'/\*__write_only\*/', r'__write_only', new_contents)
new_contents = re.sub(r'/\*pipe\*/', r'pipe', new_contents)
new_contents = re.sub(r'long \*traceBufferArray', r'__global long *traceBufferArray', new_contents)
#new_contents = re.sub(r'int \*traceBufferIdx', r'__global int *traceBufferIdx', new_contents)
newClFileName = re.sub("\.cl", "_new.cl", clFileName)
clFile = open(newClFileName, "w")
clFile.write(new_contents)
clFile.close()
