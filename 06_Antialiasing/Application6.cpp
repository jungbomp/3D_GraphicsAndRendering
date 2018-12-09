// Application6.cpp: implementation of the Application6 class.
//
//////////////////////////////////////////////////////////////////////

/*
* application test code for homework assignment #5
*/

#include "stdafx.h"
#include "CS580HW.h"
#include "Application6.h"
#include "Gz.h"
#include "rend.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

#define INFILE  "ppot.asc"
#define OUTFILE "output.ppm"


extern int tex_fun(float u, float v, GzColor color); /* image texture function */
extern int ptex_fun(float u, float v, GzColor color); /* procedural texture function */
extern int GzFreeTexture();

void shade(GzCoord norm, GzCoord color);

/* X, Y, coef */
float AAFilter[AAKERNEL_SIZE][3] = {
	-0.52,	0.38,	0.128,
	0.41,	0.56,	0.119,
	0.27,	0.08,	0.294,
	-0.17, -0.29,	0.249,
	0.58, -0.55,	0.104,
	-0.31, -0.71,	0.106
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Application6::Application6()
{

}

Application6::~Application6()
{
	Clean();
}

int Application6::Initialize()
{
	GzCamera	camera;
	int		    xRes, yRes;	/* display parameters */

	GzToken		nameListShader[9]; 	    /* shader attribute names */
	GzPointer   valueListShader[9];		/* shader attribute pointers */
	GzToken     nameListLights[10];		/* light info */
	GzPointer   valueListLights[10];
	GzToken		nameListAntiAliasing[2];	/* anti-aliasing attribute names */
	GzPointer	valueListAntiAliasing[2];	/* anti-aliasing attribute pointers */
	int			shaderType, interpStyle;
	float		specpower;
	int		status;

	status = 0;

	/*
	* Allocate memory for user input
	*/
	m_pUserInput = new GzInput;

	/*
	* initialize the display and the renderer
	*/
	m_nWidth = 256;		// frame buffer and display width
	m_nHeight = 256;    // frame buffer and display height

	for (int i = 0; i < AAKERNEL_SIZE; i++) {
		m_pRender[i] = new GzRender(m_nWidth, m_nHeight);
		m_pRender[i]->GzDefault();
	}

	// Allocate frame buffer for the anti-alising
	m_pFrameBuffer = new char[m_nWidth * m_nHeight * 3];
	//m_pFrameBuffer = m_pRender[0]->framebuffer;

	// Allocate pixel buffer for the Anti-alising
	m_pixelbuffer = new GzPixel[m_nWidth * m_nHeight];

	/* Translation matrix */
	GzMatrix	scale =
	{
		3.25,	0.0,	0.0,	0.0,
		0.0,	3.25,	0.0,	-3.25,
		0.0,	0.0,	3.25,	3.5,
		0.0,	0.0,	0.0,	1.0
	};

	GzMatrix	rotateX =
	{
		1.0,	0.0,	0.0,	0.0,
		0.0,	.7071,	.7071,	0.0,
		0.0,	-.7071,	.7071,	0.0,
		0.0,	0.0,	0.0,	1.0
	};

	GzMatrix	rotateY =
	{
		.866,	0.0,	-0.5,	0.0,
		0.0,	1.0,	0.0,	0.0,
		0.5,	0.0,	.866,	0.0,
		0.0,	0.0,	0.0,	1.0
	};

#if 1 	/* set up app-defined camera if desired, else use camera defaults */
	camera.position[X] = -3;
	camera.position[Y] = -25;
	camera.position[Z] = -4;

	camera.lookat[X] = 7.8;
	camera.lookat[Y] = 0.7;
	camera.lookat[Z] = 6.5;

	camera.worldup[X] = -0.2;
	camera.worldup[Y] = 1.0;
	camera.worldup[Z] = 0.0;

	camera.FOV = 63.7;              /* degrees *              /* degrees */

	for (int i = 0; i < AAKERNEL_SIZE; i++) {
		status |= m_pRender[i]->GzPutCamera(camera);
	}
#endif 

	/* Start Renderer */
	for (int i = 0; i < AAKERNEL_SIZE; i++) {
		status |= m_pRender[i]->GzBeginRender();
	}

	/* Light */
	GzLight	light1 = { { -0.7071, 0.7071, 0 },{ 0.5, 0.5, 0.9 } };
	GzLight	light2 = { { 0, -0.7071, -0.7071 },{ 0.9, 0.2, 0.3 } };
	GzLight	light3 = { { 0.7071, 0.0, -0.7071 },{ 0.2, 0.7, 0.3 } };
	GzLight	ambientlight = { { 0, 0, 0 },{ 0.3, 0.3, 0.3 } };

	/* Material property */
	GzColor specularCoefficient = { 0.3, 0.3, 0.3 };
	GzColor ambientCoefficient = { 0.1, 0.1, 0.1 };
	GzColor diffuseCoefficient = { 0.7, 0.7, 0.7 };

	/*
	renderer is ready for frame --- define lights and shader at start of frame
	*/

	/*
	* Tokens associated with light parameters
	*/
	nameListLights[0] = GZ_DIRECTIONAL_LIGHT;
	valueListLights[0] = (GzPointer)&light1;
	nameListLights[1] = GZ_DIRECTIONAL_LIGHT;
	valueListLights[1] = (GzPointer)&light2;
	nameListLights[2] = GZ_DIRECTIONAL_LIGHT;
	valueListLights[2] = (GzPointer)&light3;
	for (int i = 0; i < AAKERNEL_SIZE; i++) {
		status |= m_pRender[i]->GzPutAttribute(3, nameListLights, valueListLights);
	}

	nameListLights[0] = GZ_AMBIENT_LIGHT;
	valueListLights[0] = (GzPointer)&ambientlight;
	for (int i = 0; i < AAKERNEL_SIZE; i++) {
		status |= m_pRender[i]->GzPutAttribute(1, nameListLights, valueListLights);
	}

	/*
	* Tokens associated with shading
	*/
	nameListShader[0] = GZ_DIFFUSE_COEFFICIENT;
	valueListShader[0] = (GzPointer)diffuseCoefficient;

	/*
	* Select either GZ_COLOR or GZ_NORMALS as interpolation mode
	*/
	nameListShader[1] = GZ_INTERPOLATE;
	//interpStyle = GZ_COLOR;         /* Gouraud shading */
	interpStyle = GZ_NORMALS;         /* Phong shading */
	valueListShader[1] = (GzPointer)&interpStyle;

	nameListShader[2] = GZ_AMBIENT_COEFFICIENT;
	valueListShader[2] = (GzPointer)ambientCoefficient;
	nameListShader[3] = GZ_SPECULAR_COEFFICIENT;
	valueListShader[3] = (GzPointer)specularCoefficient;
	nameListShader[4] = GZ_DISTRIBUTION_COEFFICIENT;
	specpower = 32;
	valueListShader[4] = (GzPointer)&specpower;

	nameListShader[5] = GZ_TEXTURE_MAP;
#if 0   /* set up null texture function or valid pointer */
	valueListShader[5] = (GzPointer)0;
#else
	valueListShader[5] = (GzPointer)(ptex_fun);	/* or use ptex_fun */
#endif

	for (int i = 0; i < AAKERNEL_SIZE; i++) {
		status |= m_pRender[i]->GzPutAttribute(6, nameListShader, valueListShader);
	}

	for (int i = 0; i < AAKERNEL_SIZE; i++) {
		nameListAntiAliasing[0] = GZ_AASHIFTX;
		valueListAntiAliasing[0] = (GzPointer)&(AAFilter[i][X]);
		nameListAntiAliasing[1] = GZ_AASHIFTY;
		valueListAntiAliasing[1] = (GzPointer)&(AAFilter[i][Y]);
		status |= m_pRender[i]->GzPutAttribute(2, nameListAntiAliasing, valueListAntiAliasing);

		status |= m_pRender[i]->GzPushMatrix(scale);
		status |= m_pRender[i]->GzPushMatrix(rotateY);
		status |= m_pRender[i]->GzPushMatrix(rotateX);
	}

	if (status) exit(GZ_FAILURE);

	if (status)
		return(GZ_FAILURE);
	else
		return(GZ_SUCCESS);
}

int Application6::Render()
{
	GzToken		nameListTriangle[3]; 	/* vertex attribute names */
	GzPointer	valueListTriangle[3]; 	/* vertex attribute pointers */
	GzCoord		vertexList[3];	/* vertex position coordinates */
	GzCoord		normalList[3];	/* vertex normals */
	GzTextureIndex  	uvList[3];		/* vertex texture map indices */
	char		dummy[256];
	int			status;


	/* Initialize Display */
	for (int i = 0; i < AAKERNEL_SIZE; i++) {
		status |= m_pRender[i]->GzDefault();  /* init for new frame */
	}

	/*
	* Tokens associated with triangle vertex values
	*/
	nameListTriangle[0] = GZ_POSITION;
	nameListTriangle[1] = GZ_NORMAL;
	nameListTriangle[2] = GZ_TEXTURE_INDEX;

	// I/O File open
	FILE *infile;
	if ((infile = fopen(INFILE, "r")) == NULL)
	{
		AfxMessageBox("The input file was not opened\n");
		return GZ_FAILURE;
	}

	FILE *outfile;
	if ((outfile = fopen(OUTFILE, "wb")) == NULL)
	{
		AfxMessageBox("The output file was not opened\n");
		return GZ_FAILURE;
	}

	/*
	* Walk through the list of triangles, set color
	* and render each triangle
	*/
	while (fscanf(infile, "%s", dummy) == 1) { 	/* read in tri word */
		fscanf(infile, "%f %f %f %f %f %f %f %f",
			&(vertexList[0][0]), &(vertexList[0][1]),
			&(vertexList[0][2]),
			&(normalList[0][0]), &(normalList[0][1]),
			&(normalList[0][2]),
			&(uvList[0][0]), &(uvList[0][1]));
		fscanf(infile, "%f %f %f %f %f %f %f %f",
			&(vertexList[1][0]), &(vertexList[1][1]),
			&(vertexList[1][2]),
			&(normalList[1][0]), &(normalList[1][1]),
			&(normalList[1][2]),
			&(uvList[1][0]), &(uvList[1][1]));
		fscanf(infile, "%f %f %f %f %f %f %f %f",
			&(vertexList[2][0]), &(vertexList[2][1]),
			&(vertexList[2][2]),
			&(normalList[2][0]), &(normalList[2][1]),
			&(normalList[2][2]),
			&(uvList[2][0]), &(uvList[2][1]));

		/*
		* Set the value pointers to the first vertex of the
		* triangle, then feed it to the renderer
		* NOTE: this sequence matches the nameList token sequence
		*/
		valueListTriangle[0] = (GzPointer)vertexList;
		valueListTriangle[1] = (GzPointer)normalList;
		valueListTriangle[2] = (GzPointer)uvList;

		for (int i = 0; i < AAKERNEL_SIZE; i++) {
			m_pRender[i]->GzPutTriangle(3, nameListTriangle, valueListTriangle);
		}
	}

	NormalizePixelBuffer(); // Normalize all pixel buffers in renders using AAFilter weight values.
	GzFlushDisplay2File(outfile); 	/* write out or update display to file*/
	GzFlushDisplay2FrameBuffer();	// write out or update display to frame buffer

	/*
	* Close file
	*/

	if (fclose(infile))
		AfxMessageBox(_T("The input file was not closed\n"));

	if (fclose(outfile))
		AfxMessageBox(_T("The output file was not closed\n"));

	if (status)
		return(GZ_FAILURE);
	else
		return(GZ_SUCCESS);
}

int Application6::Clean()
{
	/*
	* Clean up and exit
	*/
	int	status = 0;

	for (int i = 0; i < AAKERNEL_SIZE; i++) {
		delete m_pRender[i];
		m_pRender[i] = NULL;
	}
	status |= GzFreeTexture();

	delete[] m_pixelbuffer;
	m_pixelbuffer = NULL;

	delete[] m_pFrameBuffer;
	m_pFrameBuffer = NULL;

	if (status)
		return(GZ_FAILURE);
	else
		return(GZ_SUCCESS);
}


int Application6::NormalizePixelBuffer()
{
	if (!m_pixelbuffer)
		return GZ_FAILURE;

	for (int i = 0; i < AAKERNEL_SIZE; i++) {
		if (!m_pRender[i]) return GZ_FAILURE;
	}

	for (int i = 0, j = 0; i < m_nWidth * m_nWidth; i++) {
		m_pixelbuffer[i].red = 0;
		m_pixelbuffer[i].green = 0;
		m_pixelbuffer[i].blue = 0;
		m_pixelbuffer[i].alpha = 0;

		for (int k = 0; k < AAKERNEL_SIZE; k++) {
			m_pixelbuffer[i].red += static_cast<GzIntensity>(static_cast<float>(m_pRender[k]->pixelbuffer[i].red) * AAFilter[k][2]);
			m_pixelbuffer[i].green += static_cast<GzIntensity>(static_cast<float>(m_pRender[k]->pixelbuffer[i].green) * AAFilter[k][2]);
			m_pixelbuffer[i].blue += static_cast<GzIntensity>(static_cast<float>(m_pRender[k]->pixelbuffer[i].blue) * AAFilter[k][2]);
			m_pixelbuffer[i].alpha += static_cast<GzIntensity>(static_cast<float>(m_pRender[k]->pixelbuffer[i].alpha) * AAFilter[k][2]);
		}
	}

	return GZ_SUCCESS;
}

int Application6::GzFlushDisplay2File(FILE* outfile)
{
	if (!m_pixelbuffer)
		return GZ_FAILURE;

	char* buf = new char[m_nWidth * m_nHeight * 3];
	memset(buf, '\0', sizeof(char) * m_nWidth * m_nHeight * 3);

	for (int i = 0, j = 0; i < m_nWidth * m_nWidth; i++) {
		buf[j++] = (0x0FFF < m_pixelbuffer[i].red ? 0xFF : (m_pixelbuffer[i].red >> 4));
		buf[j++] = (0x0FFF < m_pixelbuffer[i].green ? 0xFF : (m_pixelbuffer[i].green >> 4));
		buf[j++] = (0x0FFF < m_pixelbuffer[i].blue ? 0xFF : (m_pixelbuffer[i].blue >> 4));
	}

	fprintf(outfile, "P6 %d %d %d\n", m_nWidth, m_nHeight, 255);
	fwrite(buf, sizeof(char), m_nWidth*m_nHeight * 3, outfile);
	delete[] buf;

	return GZ_SUCCESS;
}

int Application6::GzFlushDisplay2FrameBuffer()
{
	if (!m_pFrameBuffer)
		return GZ_FAILURE;

	for (int i = 0, j = 0; i < m_nWidth * m_nHeight; i++) {
		m_pFrameBuffer[j++] = (0x0FFF < m_pixelbuffer[i].blue ? 0xFF : (m_pixelbuffer[i].blue >> 4));
		m_pFrameBuffer[j++] = (0x0FFF < m_pixelbuffer[i].green ? 0xFF : (m_pixelbuffer[i].green >> 4));
		m_pFrameBuffer[j++] = (0x0FFF < m_pixelbuffer[i].red ? 0xFF : (m_pixelbuffer[i].red >> 4));
	}

	return GZ_SUCCESS;
}
