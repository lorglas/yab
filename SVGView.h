#ifndef _SVGVIEW_H_
#define _SVGVIEW_H_

#include <String.h>
#include <View.h>

// typedef to remove dependence on expat.h
struct XML_ParserStruct;
typedef struct XML_ParserStruct *XML_Parser;

class BNode;
class BEntry;
class BFile;
class BPopUpMenu;
class BPositionIO;
class BSVGElement;
class BSVGGradient;
class BSVGPath;
class BSVGGroup;
class BSVGDocument;
class BSVGText;
class CSSParser;
class entry_ref;
class gradient_s;
class attribute_s;
class Matrix2D;
class DependentObject;
class EvenOddIterator;

class BSVGView : public BView {

public:
						BSVGView(	BRect frame,
									const char *name,
									uint32 resizeMask,
									uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);
virtual					~BSVGView();

						BSVGView(BMessage *data);
static	BArchivable		*Instantiate(BMessage *data);
virtual	status_t		Archive(BMessage *data, bool deep = true) const;

		status_t		LoadFromFile(const char *name);
		status_t		LoadFromFile(BEntry *entry);
		status_t		LoadFromFile(entry_ref *ref);
		status_t		LoadFromFile(BFile *file);
		status_t		LoadFromAttribute(	const char *filename,
											const char *attribute);
		status_t		LoadFromAttribute(	BEntry *entry,
											const char *attribute);
		status_t		LoadFromAttribute(	entry_ref *ref,
											const char *attribute);
		status_t		LoadFromAttribute(	BNode *node,
											const char *attribute);
		status_t		LoadFromPositionIO(BPositionIO *positionio);
		status_t		LoadFromBuffer(char *buffer, off_t length);
		status_t		LoadFromString(BString &string);

		status_t		ExportToFile(const char *name);
		status_t		ExportToFile(BEntry *entry);
		status_t		ExportToFile(entry_ref *ref);
		status_t		ExportToFile(BFile *file);
		status_t		ExportToAttribute(	const char *filename,
											const char *attribute);
		status_t		ExportToAttribute(	BEntry *entry,
											const char *attribute);
		status_t		ExportToAttribute(	entry_ref *ref,
											const char *attribute);
		status_t		ExportToAttribute(	BNode *node,
											const char *attribute);
		status_t		ExportToPositionIO(BPositionIO *positionio);
		status_t		ExportToString(BString &data);

		void			SavePanel();
		void			ShowSource();

virtual	void			AddElement(BSVGElement *element);
		BSVGElement		*RemoveElement(const char *id, bool deep = true);
		BSVGElement		*RemoveElement(BSVGElement *element, bool deep = true);
		BSVGElement		*RemoveElementAt(int index, bool deep = true);

		BSVGElement		*ElementAt(int index, bool deep = true);
		int32			CountElements(bool deep = true);

		BSVGElement		*FindElement(const BString &id, int32 of_type = -1);
		BSVGElement		*FindElement(const char *id, int32 of_type = -1);

		int32			FindElements(const char *_class, BSVGElement *elements[]);
		int32			FindElements(const BString &_class, BSVGElement *elements[]);

		void			ClearElements();
		void			Clear();

virtual	void			SetAttributeByClass(const BString &_class, const BString &attribute, const BString &value);
virtual	void			SetAttributeByID(const BString &id, const BString &attribute, const BString &value);
virtual	void			SetTextByClass(const BString &_class, const BString &text);
virtual	void			SetTextByID(const BString &id, const BString &text);

// configuration
virtual	void			SetScale(float scale);
		float			Scale() const;
virtual	void			SetScaleToFit(bool enabled);
		bool			ScaleToFit() const;
virtual void			SetFitContent(bool enabled);
		bool			FitContent() const;
virtual	void			SetSuperSampling(bool enabled);
		bool			SuperSampling() const;
virtual	void			SetSampleSize(uint32 samplesize);
		uint32			SampleSize() const;
virtual	void			SetFocusPoint(BPoint focus, bool in_views_system, bool percentage);
		BPoint			FocusPoint(bool as_percentage = false) const;
virtual	void			SetViewCenter(BPoint center, bool percentage);
		BPoint			ViewCenter(bool as_percentage = false) const;
virtual	void			SetPulseRate(bigtime_t rate);
		bigtime_t		PulseRate() const;

// zooming - zooming will just adjust the scale and focus point for you
virtual	void			ZoomIn(BPoint focus, bool in_views_system = true, bool percentage = false);
virtual	void			ZoomOut(BPoint focus, bool in_views_system = true, bool percentage = false);
virtual	void			ResetZoom();

// interface stuff
virtual	void			SetDropsEnabled(bool enabled);
		bool			DropsEnabled() const;
virtual	void			SetMenusEnabled(bool enabled);
		bool			MenusEnabled() const;
		BPopUpMenu		*ContextMenu(); // edit this menu but don't delete it

		float			GraphicsWidth() const;
		float			GraphicsHeight() const;

virtual	void			SetViewColor(rgb_color color);
		void			SetViewColor(uchar r, uchar g, uchar b, uchar a = 255);
		rgb_color		ViewColor() const;

		void			LockUpdates(); // delay individual updates
		void			UnlockUpdates(bool redraw = false);

		void			Redraw(bool force = false); // force even if locked
virtual	void			Draw(BRect updateRect);
virtual	void			FrameResized(float new_width, float new_height);
virtual	void			MessageReceived(BMessage *message);
virtual	void			MouseDown(BPoint where);

		BBitmap			*NeedTempBitmap(BRect frame);
		BView			*NeedTempView(BRect frame);
		EvenOddIterator	*NeedEvenOdd(BRect frame);
static	void			StripWhiteSpaces(BString *string);
static	Matrix2D		HandleTransformation(BString &value);

private:
		BSVGDocument	*fDocument;

		XML_Parser		fParser;
		CSSParser		*fCSSParser;
static	void			XMLHandlerStart(void *, const char *, const char **);
static	void			XMLHandlerEnd(void *, const char *);
static	void			XMLHandlerText(void *, const char *, int);
static	int32			PulseThreadFunc(void *);

		void			FlushText();
		void			UpdateInnerFrame();
		void			PrepareTempBitmap();
		void			FreeTempBitmap();
		void			ScaleOffscreen();

		status_t		InitializeXMLParser();
		status_t		UninitializeXMLParser();

		status_t		ParseXMLData(BPositionIO *data);
		status_t		ParseXMLFile(BFile *file);
		status_t		ParseXMLNode(BNode *node, const char *attr);
		status_t		ParseXMLBuffer(char *buffer, off_t size);

		bool			fScaleToFit;
		bool			fFitContent;
		bool			fSuperSampling;
		uint32			fSampleSize;

		float			fScale;
		BPoint			fFocusPoint; // percents of image width/height
		BPoint			fViewCenter; // percents of view width/heigth
		float			fGraphicsWidth;
		float			fGraphicsHeight;
		BRect			fInnerFrame;
		BRect			fViewBounds;
		rgb_color		fViewColor;

		BSVGGradient	*fGradient;	// the currently parsed gradient
		BSVGText		*fText;	// the currently parsed text
		BString			fTextBuffer;

		BBitmap			*fOffscreenBitmap;
		BBitmap			*fTempBitmap;
		BView			*fTempView;
		EvenOddIterator	*fEvenOdd;

		bool			fOuterFirst;
		BRect			fOuterBounds;
		
		bool			fInsideCSS;

		bool			fUpdateNeeded;
		BRect			fSourceRect;
		BPoint			fSourceOrigin;

		int				fTransparentHack;
		bigtime_t		fPulseRate;
		thread_id		fPulseThread;

		bool			fDropsEnabled;
		bool			fMenusEnabled;
		BPopUpMenu		*fContextMenu;

		bool			fLocked;

// reserve some space
		uint32			_fReserved[100];
};

#endif	// SVGVIEW_H_
