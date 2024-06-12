Ben Hulse
COMP5823M Animation and Simulation
Assignment 1

# The .2fi file format

The file type used for 2D files is .2fi and is as follows:
The first two numbers are integers representing the number of vertices and faces respectively.
The second block contains a set of two floats per vertex, representing the x and y coordinates of said vertex.
The final block in the file contains three integers per face, each an index of the vertices at the corners of the triangular face. These vertices should be listed in counter clockwise order.
In the example I use "//" to denote a comment explanation, these should not be present in the actual file.

Exmaple.2fi:
-------------
4 // The shape has 4 vertices
2 // The shape consists of two triangular faces

1 1 // Vertex 0 has coordinates (1,1)
-1 1
-1 -1
1 -1

0 1 2 // Face 0 has corners at vertices 0, 1 and 2
0 1 3
------------

The file should only contain the relevant integers and floats, separated by some white space. It is not needed that the verts/faces occupy their own line, in fact the whole file can be on one line, as long as there is white space between separating the numbers.

# Folder structure

The program will ony be able to find files in the "/files/" folder within the project folder. Additionally all output files will be saved ot this folder with the same name as the original file, prefixed with "edited_".