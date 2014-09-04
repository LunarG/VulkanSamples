//  XGL tests
//
//  Copyright (C) 2014 LunarG, Inc.
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.

#include "xgltestframework.h"
#include "GL/freeglut_std.h"

XglTestFramework::XglTestFramework() :
    m_glut_initialized( false )
{
}

// Define all the static elements
bool XglTestFramework::m_show_images = false;
bool XglTestFramework::m_save_images = false;
int XglTestFramework::m_width = 0;
int XglTestFramework::m_height = 0;
int XglTestFramework::m_window = 0;
std::list<XglTestImageRecord> XglTestFramework::m_images;
std::list<XglTestImageRecord>::iterator XglTestFramework::m_display_image;
int m_display_image_idx = 0;

void XglTestFramework::InitArgs(int *argc, char *argv[])
{
    int i, n;

    for (i=0, n=0; i< *argc; i++) {
        if (strncmp("--show-images", argv[i], 13) == 0) {
            m_show_images = true;
            continue;
        }
        if (strncmp("--save-images", argv[i], 13) == 0) {
            m_save_images = true;
            continue;
        }

        /*
         * Since the above "consume" inputs, update argv
         * so that it contains the trimmed list of args for glutInit
         */
        argv[n] = argv[i];
        n++;
    }

    if (m_show_images) {
        glutInit(argc, argv);
    }
}

void XglTestFramework::Reshape( int w, int h )
{
    if (!m_show_images) return;  // Do nothing except save info if not enabled

    // Resize window to be large enough to handle biggest image we've seen
    // TODO: Probably need some sort of limits for the Window system.
    if (w > m_width) {
        m_width = w;
    }
    if (h > m_height) {
        m_height = h;
    }

    glutReshapeWindow(m_width, m_height);

    glViewport( 0, 0, m_width, m_height );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( 0.0, m_width, 0.0, m_height, 0.0, 2.0 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

//    glScissor(width/4, height/4, width/2, height/2);
}

void XglTestFramework::WritePPM( const char *basename, XglImage *image )
{
    string filename;
    XGL_RESULT err;
    int x, y;

    filename.append(basename);
    filename.append(".ppm");

    const XGL_IMAGE_SUBRESOURCE sr = {
        XGL_IMAGE_ASPECT_COLOR, 0, 0
    };
    XGL_SUBRESOURCE_LAYOUT sr_layout;
    XGL_UINT data_size;

    err = xglGetImageSubresourceInfo( image->image(), &sr,
                                      XGL_INFO_TYPE_SUBRESOURCE_LAYOUT,
                                      &data_size, &sr_layout);
    ASSERT_XGL_SUCCESS( err );
    ASSERT_EQ(data_size, sizeof(sr_layout));

    const char *ptr;

    err = xglMapMemory( image->memory(), 0, (XGL_VOID **) &ptr );
    ASSERT_XGL_SUCCESS( err );

    ptr += sr_layout.offset;

    ofstream file (filename.c_str());
    ASSERT_TRUE(file.is_open()) << "Unable to open file: " << filename;

    file << "P6\n";
    file << image->width() << "\n";
    file << image->height() << "\n";
    file << 255 << "\n";

    for (y = 0; y < image->height(); y++) {
        const char *row = ptr;

        for (x = 0; x < image->width(); x++) {
            file.write(row, 3);
            row += 4;
        }

        ptr += sr_layout.rowPitch;
    }

    file.close();

    err = xglUnmapMemory( image->memory() );
    ASSERT_XGL_SUCCESS( err );
}

void XglTestFramework::InitGLUT(int w, int h)
{

    if (!m_show_images) return;

    if (!m_glut_initialized) {
        glutInitWindowSize(w, h);

        glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
        m_window = glutCreateWindow(NULL);
        m_glut_initialized = true;
    }

    Reshape(w, h);
}

void XglTestFramework::Show(const char *comment, XglImage *image)
{
    XGL_RESULT err;

    const XGL_IMAGE_SUBRESOURCE sr = {
        XGL_IMAGE_ASPECT_COLOR, 0, 0
    };
    XGL_SUBRESOURCE_LAYOUT sr_layout;
    XGL_UINT data_size;

    if (!m_show_images) return;

    InitGLUT(image->width(), image->height());

    err = xglGetImageSubresourceInfo( image->image(), &sr, XGL_INFO_TYPE_SUBRESOURCE_LAYOUT,
                                      &data_size, &sr_layout);
    ASSERT_XGL_SUCCESS( err );
    ASSERT_EQ(data_size, sizeof(sr_layout));

    const char *ptr;

    err = image->MapMemory( (XGL_VOID **) &ptr );
    ASSERT_XGL_SUCCESS( err );

    ptr += sr_layout.offset;

    XglTestImageRecord record;
    record.m_title.append(comment);
    record.m_width = image->width();
    record.m_height = image->height();
    // TODO: Need to make this more robust to handle different image formats
    record.m_data_size = image->width()*image->height()*4;
    record.m_data = malloc(record.m_data_size);
    memcpy(record.m_data, ptr, record.m_data_size);
    m_images.push_back(record);
    m_display_image = --m_images.end();

    Display();

    err = image->UnmapMemory();
    ASSERT_XGL_SUCCESS( err );
}

void XglTestFramework::RecordImage(XglImage *image)
{
    const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();

    if (m_save_images) {
        WritePPM(test_info->test_case_name(), image);
    }

    if (m_show_images) {
        Show(test_info->test_case_name(), image);
    }
}

void XglTestFramework::Display()
{
    glutSetWindowTitle(m_display_image->m_title.c_str());

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glRasterPos3f(0, 0, 0);
    glBitmap(0, 0, 0, 0, 0, 0, NULL);
    glDrawPixels(m_display_image->m_width, m_display_image->m_height,
                 GL_RGBA, GL_UNSIGNED_BYTE, m_display_image->m_data);

    glutSwapBuffers();
}

void XglTestFramework::Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         glutDestroyWindow(m_window);
         exit(0);
         break;
   }
   glutPostRedisplay();
}

void XglTestFramework::SpecialKey( int key, int x, int y )
{
    (void) x;
    (void) y;
    switch (key) {
    case GLUT_KEY_UP:
    case GLUT_KEY_RIGHT:
        ++m_display_image;
        if (m_display_image == m_images.end()) {
            m_display_image = m_images.begin();
        }
        break;
    case GLUT_KEY_DOWN:
    case GLUT_KEY_LEFT:
        if (m_display_image == m_images.begin()) {
            m_display_image = --m_images.end();
        } else {
            --m_display_image;
        }

        break;
    }
    glutPostRedisplay();
}

void XglTestFramework::Finish()
{
    if (m_images.size() == 0) return;

    glutReshapeFunc( Reshape );
    glutKeyboardFunc( Key );
    glutSpecialFunc( SpecialKey );
    glutDisplayFunc( Display );

    glutMainLoop();
}



XglTestImageRecord::XglTestImageRecord() : // Constructor
    m_width( 0 ),
    m_height( 0 ),
    m_data( NULL )
{
    m_data_size = 0;
}

XglTestImageRecord::~XglTestImageRecord()
{

}

XglTestImageRecord::XglTestImageRecord(const XglTestImageRecord &copyin)   // Copy constructor to handle pass by value.
{
    m_title = copyin.m_title;
    m_width = copyin.m_width;
    m_height = copyin.m_height;
    m_data_size = copyin.m_data_size;
    m_data = copyin.m_data; // TODO: Do we need to copy the data or is pointer okay?
}

ostream &operator<<(ostream &output, const XglTestImageRecord &XglTestImageRecord)
{
    output << XglTestImageRecord.m_title << " (" << XglTestImageRecord.m_width <<
              "," << XglTestImageRecord.m_height << ")" << endl;
    return output;
}

XglTestImageRecord& XglTestImageRecord::operator=(const XglTestImageRecord &rhs)
{
    m_title = rhs.m_title;
    m_width = rhs.m_width;
    m_height = rhs.m_height;
    m_data_size = rhs.m_data_size;
    m_data = rhs.m_data;
    return *this;
}

int XglTestImageRecord::operator==(const XglTestImageRecord &rhs) const
{
    if( this->m_data != rhs.m_data) return 0;
    return 1;
}

// This function is required for built-in STL list functions like sort
int XglTestImageRecord::operator<(const XglTestImageRecord &rhs) const
{
    if( this->m_data_size < rhs.m_data_size ) return 1;
    return 0;
}

