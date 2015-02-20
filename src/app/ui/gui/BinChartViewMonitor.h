#if false
//===========================================================================
// TBinChartWindow
//===========================================================================
class TBinChartWindow : public ChartMonitorView
{
public:
    TBinChartWindow( const char* name, const char* settingsName );
        
    void SetExponent(float e);
    float GetExponent();
    
    void addPoint(const float* val, long numval);
    
    virtual void RestoreFromPrefs(long x, long y);
        
    virtual void Dump(std::ostream& out);
    virtual void Load(std::istream& in);

protected:
	virtual void Init();
	virtual void plotPoints();
	void plotPoint(long x, const float* y);

	long numbins;
	float* fy;
	float myexp;

 private:
	class ChartMonitor *monitor;
};


//===========================================================================
// inlines
//===========================================================================
inline void TBinChartWindow::SetExponent(float e) { myexp = e; }
inline float TBinChartWindow::GetExponent() { return myexp; }

#endif

// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#if false
//===========================================================================
// TBinChartWindow
//===========================================================================

//---------------------------------------------------------------------------
// TBinChartWindow::TBinChartWindow
//---------------------------------------------------------------------------
TBinChartWindow::TBinChartWindow( const char* name, const char* settingsName )
	:	ChartMonitorView( name, settingsName )
{
	Init();
}


//---------------------------------------------------------------------------
// TBinChartWindow::RestoreFromPrefs
//---------------------------------------------------------------------------
void TBinChartWindow::RestoreFromPrefs(long x, long y)
{
	// Set up some defaults
	int defWidth = kMaxWidth;
	int defHeight = kMaxHeight;
	int defX = x;
	int defY = y;
	bool visible = true;
	int titleHeight = 16;
	
	// Attempt to restore window size and position from prefs
	// Save size and location to prefs
	QSettings settings;

	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );
		
			defWidth = settings.value( "width" ).toInt();
			defHeight = settings.value( "height" ).toInt();
			defX = settings.value( "x" ).toInt();
			defY = settings.value( "y" ).toInt();
			visible = settings.value( "visible" ).toBool();
			
		settings.endGroup();
	settings.endGroup();
	
	// Set window size and location based on prefs
 	QRect position;
 	position.setTopLeft(QPoint(defX, defY));
 	position.setSize(QSize(defWidth, defHeight));
  	setGeometry(position);
	setFixedSize(defWidth, defHeight);

	// Pin values
	if (defWidth > kMaxWidth)
		defWidth = kMaxWidth;
		
	if (defHeight > kMaxHeight)
		defHeight = kMaxHeight;
		
	QDesktopWidget *d = QApplication::desktop();
	if (defX < d->x() || defX > d->width())
		defX = 0;

	if (defY < d->y() + kMenuBarHeight + titleHeight || defY > d->height())
		defY = kMenuBarHeight + titleHeight;
		
	// Set up display defaults
    fLowX = 50;
	fHighX = width() - fLowX; // fLowX + width() - 10;
    fLowY = 10;
    fHighY = height() - fLowY - 22; // fLowY + width() - 22;
	
    fMaxPoints = fHighX - fLowX + 1;
    numbins = fHighY - fLowY + 1;
    
    dydv[0] = float(fHighY - fLowY) / (fHighV[0] - fLowV[0]);
    
    if (visible)
		show();
    
	// Save settings for future restore		
	SaveWindowState();		    
}


//---------------------------------------------------------------------------
// TBinChartWindow::Init
//---------------------------------------------------------------------------
void TBinChartWindow::Init()
{
	ChartMonitorView::Init(1);
	
    fColor[0].r = 1.0;
    fColor[0].g = 1.0;
    fColor[0].b = 1.0;
    
    fy = NULL;
    numbins = fHighY - fLowY + 1;
    myexp = 1.0;
}


//---------------------------------------------------------------------------
// TBinChartWindow::Init
//---------------------------------------------------------------------------
void TBinChartWindow::plotPoints()
{
    for (long i = 0; i < fNumPoints[0]; i++)
        plotPoint(fLowX + i, &fy[i * numbins]);
}


//---------------------------------------------------------------------------
// TBinChartWindow::plotPoint
//---------------------------------------------------------------------------
void TBinChartWindow::plotPoint(long x, const float* y)
{
	Q_CHECK_PTR(y);
	
	Color c;
    for (long i = 0; i < numbins; i++)
    {
        c.r = y[i] * fColor[0].r;
        c.g = y[i] * fColor[0].g;
        c.b = y[i] * fColor[0].b;

		glColor3fv((GLfloat *)&c.r);
		glBegin(GL_POINTS);
        	glVertex2f(x, fLowY + 1);
		glEnd();        
    }
}


//---------------------------------------------------------------------------
// TBinChartWindow::Dump
//---------------------------------------------------------------------------
void TBinChartWindow::Dump(std::ostream& out)
{
    out << fNumPoints[0] nl;
    out << numbins nl;
    for (long i = 0; i < fNumPoints[0]; i++)
    {
        for (long j = 0; j < numbins; j++)
        {
            out << fy[(i * numbins) + j] nl;
        }
    }
}


//---------------------------------------------------------------------------
// TBinChartWindow::Load
//---------------------------------------------------------------------------
void TBinChartWindow::Load(std::istream& in)
{
    in >> fNumPoints[0];
    
    if (fNumPoints[0] > fMaxPoints)
        error(2, "while loading binchartwindow from dump file, fNumPoints too big");
        
    long newnumbins;
    in >> newnumbins;
    if (newnumbins != numbins)
        error(2, "while loading binchartwindow from dump file, numbins redefined");
        
    if (y == NULL)  // first time must allocate space
    {
        y = new long[fMaxPoints*numbins];
        Q_CHECK_PTR(y);
        fy = (float*)y;
    }
    
    for (long i = 0; i < fNumPoints[0]; i++)
    {
        for (long j = 0; j < numbins; j++)
        {
            in >> fy[(i * numbins) + j];
        }
    }
}


//---------------------------------------------------------------------------
// TBinChartWindow::addPoint
//---------------------------------------------------------------------------
void TBinChartWindow::addPoint(const float* val, long numval)
{
    long i, j, k;
    long ymx = 0;

    if (y == NULL)  // first time must allocate space
    {
		y = new long[fMaxPoints*numbins];
		Q_CHECK_PTR(y);
        fy = (float*)y;
    }

    if (fNumPoints[0] == fMaxPoints)
    {
        // cut the number (and resolution) of points stored in half
        for (i = 1, j = 2; j < fMaxPoints; i++, j+=2)
        {
            for (k = 0; k < numbins; k++)
                fy[i * numbins + k] = fy[j * numbins +k];
        }
        fNumPoints[0] = i;
        fDecimation++;
		if( isVisible() )
		{
			makeCurrent();
			draw();
		}
    }

	long koff = fNumPoints[0] * numbins;

    for (k = 0; k < numbins; k++)
        y[koff+k] = 0;

    for (i = 0; i < numval; i++)
    {
        k = (long)((val[i] - fLowV[0]) * dydv[0]);
        k = max<long>(0, min<long>(numbins - 1, k));
        y[koff + k]++;
        ymx = std::max(ymx, y[koff + k]);
    }
		
	if( isVisible() )
	{
		makeCurrent();
		plotPoint( fNumPoints[0] + fLowX, &fy[koff] );
	}
	
	fNumPoints[0]++;
}
#endif
