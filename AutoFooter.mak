##
## GCC Options
##
GCC = gcc
GCC_OPT = $(DBG) $(OPT) -I. -I/boot/home/config/include/ -I/boot/home/config/include/ncurses/ -DHAVE_CONFIG -DUNIX $(HAIKUOPT)
GPP = g++
GPP_OPT = $(DBG) $(OPT) -I. -DHAVE_CONFIG -DUNIX $(HAIKUOPT)

## flags for flex (-d for debugging)
FLEXFLAGS = -i -I -L -s

## flags for bison (-t -v for debugging)
BISONFLAGS = -d -l -t -v  

##
## Compile and link
##
yab: flex.o YabMain.o YabInterface.o YabWindow.o YabView.o YabBitmapView.o $(FILEPANEL) $(FILEPANELLOOPER) YabList.o $(YABTEXT) $(BUBBLE) bison.o symbol.o function.o graphic.o io.o main.o $(CLV) $(CLVYAB) $(CLVCOLOR) $(YABSTACKVIEW) $(SPLITPANE) $(URLVIEW) $(HAIKUTAB) $(SPINNER) $(ZETATAB) $(CALENDAR)
	$(GPP) $(GPP_OPT) -o $(TARGET) flex.o YabMain.o YabInterface.o YabWindow.o YabView.o YabBitmapView.o $(FILEPANEL) $(FILEPANELLOOPER) YabList.o $(YABTEXT) $(BUBBLE) bison.o symbol.o function.o graphic.o io.o main.o $(CLV) $(CLVYAB) $(CLVCOLOR) $(YABSTACKVIEW) $(SPLITPANE) $(URLVIEW) $(HAIKUTAB) $(SPINNER) $(ZETATAB) $(CALENDAR) $(LIBPATH) $(LIB) $(LIBGAME) $(LIBTRACKER) $(ZETALIB) $(LIBNCURSES) -lz

YabMain.o: YabMain.cpp 
	$(GPP) $(GPP_OPT) -c YabMain.cpp -o YabMain.o
YabInterface.o: YabInterface.cpp YabInterface.h global.h YabMenu.h
	$(GPP) $(GPP_OPT) -c YabInterface.cpp -o YabInterface.o
YabWindow.o: YabWindow.cpp YabWindow.h global.h
	$(GPP) $(GPP_OPT) -c YabWindow.cpp -o YabWindow.o
YabView.o: YabView.cpp YabView.h
	$(GPP) $(GPP_OPT) -c YabView.cpp -o YabView.o
YabBitmapView.o: YabBitmapView.cpp YabBitmapView.h
	$(GPP) $(GPP_OPT) -c YabBitmapView.cpp -o YabBitmapView.o
$(FILEPANEL): YabFilePanel.cpp YabFilePanel.h
	$(GPP) $(GPP_OPT) -c YabFilePanel.cpp -o YabFilePanel.o
$(FILEPANELLOOPER): YabFilePanelLooper.cpp YabFilePanelLooper.h
	$(GPP) $(GPP_OPT) -c YabFilePanelLooper.cpp -o YabFilePanelLooper.o
$(BUBBLE): BubbleHelper.cpp BubbleHelper.h
	$(GPP) $(GPP_OPT) -c BubbleHelper.cpp -o BubbleHelper.o
YabList.o: YabList.cpp YabList.h
	$(GPP) $(GPP_OPT) -c YabList.cpp -o YabList.o
$(YABTEXT): YabText.cpp YabText.h
	$(GPP) $(GPP_OPT) -c YabText.cpp -o YabText.o
bison.o: bison.c yabasic.h config.h 
	$(GCC) $(GCC_OPT) -c bison.c -o bison.o
flex.o: flex.c bison.c yabasic.h config.h
	$(GCC) $(GCC_OPT) -c flex.c -o flex.o
function.o: function.c yabasic.h config.h
	$(GCC) $(GCC_OPT) -c function.c -o function.o
io.o: io.c yabasic.h config.h
	$(GCC) $(GCC_OPT) -c io.c -o io.o
graphic.o: graphic.c yabasic.h config.h
	$(GCC) $(GCC_OPT) -c graphic.c -o graphic.o
symbol.o: symbol.c yabasic.h config.h
	$(GCC) $(GCC_OPT) -c symbol.c -o symbol.o
main.o: main.c yabasic.h config.h 
	$(GCC) $(GCC_OPT) -c main.c -o main.o
$(YABSTACKVIEW): YabStackView.cpp YabStackView.h
	$(GPP) $(GPP_OPT) -c YabStackView.cpp -o YabStackView.o
$(SPLITPANE): SplitPane.cpp SplitPane.h 
	$(GPP) $(GPP_OPT) -c SplitPane.cpp -o SplitPane.o
$(URLVIEW): URLView.cpp URLView.h
	$(GPP) $(GPP_OPT) -c URLView.cpp -o URLView.o
$(SPINNER): Spinner.cpp Spinner.h
	$(GPP) $(GPP_OPT) -c Spinner.cpp -o Spinner.o
$(CLV): column/ColumnListView.cpp column/ColumnListView.h column/ObjectList.h 
	$(GPP) $(GPP_OPT) -c column/ColumnListView.cpp -o column/ColumnListView.o
$(CLVCOLOR): column/ColorTools.cpp column/ColorTools.h 
	$(GPP) $(GPP_OPT) -c column/ColorTools.cpp -o column/ColorTools.o
$(CLVYAB): column/YabColumnType.cpp column/YabColumnType.h
	$(GPP) $(GPP_OPT) -c column/YabColumnType.cpp -o column/YabColumnType.o
$(HAIKUTAB): YabTabView.cpp YabTabView.h
	$(GPP) $(GPP_OPT) -c YabTabView.cpp -o YabTabView.o
$(CALENDAR): CalendarControl.cpp CalendarControl.h DateTextView.cpp MonthWindow.cpp MonthWindowView.cpp MouseSenseStringView.cpp
	$(GPP) $(GPP_OPT) -c CalendarControl.cpp -o CalendarControl.o

clean:
	rm -f core *.o column/*.o

