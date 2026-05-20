# Nodeplot
Plotting software that uses a node based interface.

![](/screenshot.png)

## Building
```
mkdir build && cd build
cmake ..
make
```

## Nodeplot-gui
Node based gui editor.

To create:
```
./nodeplot-gui --new <name>
```

To open:
```
./nodeplot-gui <file>
```

## Nodeplot-cmd
Command line program to write the outputs of a nodeplot file to disk.
```
./nodeplot-cmd <file>
```