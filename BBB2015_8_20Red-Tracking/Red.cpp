#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <unistd.h>
#include <fstream>
#include <fcntl.h>
#include <termios.h>
#include <iostream>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>

using namespace cv;
using namespace std;

///Global Variables
Mat image ;
Mat ref, ref1;
Mat ref_hsv, ref_hue, hist, hist_nomask;

Mat mask, points;
Point center;
int vmin = 40, vmax = 223, smin = 108, Lowthreshold = 50, bins=60; //y:vmin 40 smin 108
int img_vmin = 0, img_vmax = 165, img_smin = 0, frame_width=320, frame_height=240, xcoord, ycoord,x_mbed,y_mbed;
/// Function Headers
void Tracking(int, void* );
void Mask_Calc(int, void*);
/*
char Open(const char*, unsigned int);
void Close();
char WriteString( const char*);
int ReadString(char*,char,unsigned int);
void FlushReceiver();
int ReadStringNoTimeOut(char, char, unsigned int );
char ReadChar(char* );

int fd;

char* DEVICE_PORT = "/dev/ttyO1" ;
*/
int rows = frame_width;
int cols = frame_height;
int sizex=cols/2;
int sizey=rows/2;

int ch[] = { 0, 0 };
int ch1[]={1,0};
int ch2[] = {2,0};
int histSize = MAX( bins, 2 );
float hue_range[] = { 0, 180 };
const float* ranges = { hue_range };

Point p1= Point(cols/2-sizex/2,rows/2-sizey/2);
Size box = Size(sizex, sizey);
Rect trackWindow = Rect(p1, box);

int main( int argc, char** argv )
{

	/*
	int Ret;                                                                // Used for return values
    char Buffer[128];
    
	Ret=Open(DEVICE_PORT,9600);                                        // Open serial link at 9600 bauds
    if (Ret!=1) {                                                           // If an error occured...
        printf ("Error while opening port. Permission problem ?\n");        // ... display a message ...
        return Ret;                                                         // ... quit the application
    }
    printf ("Serial port opened successfully !\n");
    */
/// read in reference image 
ref= imread("reference_image.jpg",CV_LOAD_IMAGE_COLOR);
//imshow("1",ref);

///Open video stream and check
VideoCapture cap(0);
//VideoCapture cap("right4.avi");
bool camera = cap.isOpened();
while( camera==false)
{
	cap.open(0);
	cout << "Trying to find camera" << endl;
	camera = cap.isOpened();
	sleep(2);
}
cap.set(CV_CAP_PROP_FRAME_WIDTH, frame_width);
cap.set(CV_CAP_PROP_FRAME_HEIGHT,frame_height);

cvtColor(ref, ref_hsv, CV_BGR2HSV);
Mask_Calc(0,0);
//mixChannels( &ref_hsv, 1, &ref_hue, 1, ch, 1 );
//calcHist( &ref_hue, 1, 0, mask, hist, 1, &histSize, &ranges, true, false );
//normalize( hist, hist, 0, 255, NORM_MINMAX, -1, Mat() );
imshow("2", mask);

while(true){
	
///capture the frame to analyize 
cap.read(image);
if (image.empty()) break;

Tracking(0,0);

/*
char out [50];
sprintf(out, "%d,%d\n\r",xcoord,ycoord);

 Ret=WriteString(out);                                             // Send the command on the serial port
    if (Ret!=1) {                                                           // If the writting operation failed ...
        printf ("Error while writing data\n");                              // ... display a message ...
        return Ret;                                                         // ... quit the application.
    }
    printf ("Write operation is successful \n");
*/

//imshow("Tracker", image);
//imshow("Reference", ref);

///wait for esc key to exit
/*
char c = (char)waitKey(10);
if( c == 27 ){
return 0;}
if( c==115)
{
imwrite("reference_image1.jpg", image);
ref1 = imread("/reference_image1.jpg",1);
//imshow("", ref);
waitKey(1000);
}
*/
//int percent_x=(xcoord/frame_width)-.5; //if greater than 1 then nothing detected
//int percent_y=(ycoord/frame_height)-.5;//if greater than 1 then nothing detected
waitKey(10);
}
}

void Tracking(int, void*){
Mat image_hsv, image_hue, image_sat, image_val, image_mask, back_proj, backproj, image_hsv_prime;
//Mat ref_hue, hist, hist_nomask;

///convert reference and frame capture to HSV	
//cvtColor(ref, ref_hsv, CV_BGR2HSV);
cvtColor(image, image_hsv_prime, CV_BGR2HSV);
image_mask=Mat::zeros(image_hsv_prime.size(), image_hsv_prime.type());
inRange(image_hsv_prime, Scalar(0,img_smin,img_vmin,0), Scalar(180, 256,img_vmax,0), image_mask);
image_mask=Mat::ones(image_mask.size(), image_mask.type())*255-image_mask;
image_hsv_prime.copyTo(image_hsv, image_mask);
///Extract the hue channel
image_hue.create( image_hsv.size(), image_hsv.depth() );
image_sat.create( image_hsv.size(), image_hsv.depth() );
image_val.create( image_hsv.size(), image_hsv.depth() );
ref_hue.create (ref_hsv.size(), ref_hsv.depth());
mixChannels( &image_hsv, 1, &image_hue, 1, ch, 1 );
mixChannels( &image_hsv, 1, &image_sat, 1, ch1, 1 );
mixChannels( &image_hsv, 1, &image_val, 1, ch2, 1 );
//imshow("hue", image_hue);
//imshow("Prime", image_hsv_prime);
//imshow("hsv", image_hsv);
//imshow("imagemaksHue", image_mask);
mixChannels( &ref_hsv, 1, &ref_hue, 1, ch, 1 );
calcHist( &ref_hue, 1, 0, mask, hist, 1, &histSize, &ranges, true, false );
normalize( hist, hist, 0, 255, NORM_MINMAX, -1, Mat() );
///calculate the reference histogram for backProjection (with and without mask to test)

//calcHist( &ref_hue, 1, 0, Mat(), hist_nomask, 1, &histSize, &ranges, true, false );
//normalize( hist_nomask, hist_nomask, 0, 255, NORM_MINMAX, -1, Mat() );

///calc back projection
calcBackProject( &image_hue, 1, 0, hist, backproj, &ranges, 1, true );
erode(backproj, backproj, Mat());
dilate(backproj, backproj, Mat());

imshow("3", backproj);
/// Draw the histogram
/*
int w = 400; int h = 400;
int bin_w = cvRound( (double) w / histSize );
Mat histImg = Mat::zeros( w, h, CV_8UC3 );
Mat hist_nomask_img = Mat::zeros( w, h, CV_8UC3 );
for( int i = 0; i < bins; i ++ )
{ rectangle( histImg, Point( i*bin_w, h ), Point( (i+1)*bin_w, h - cvRound( hist.at<float>(i)*h/255.0 ) ), Scalar( 0, 0, 255 ), -1 ); 
rectangle( hist_nomask_img, Point( i*bin_w, h ), Point( (i+1)*bin_w, h - cvRound( hist_nomask.at<float>(i)*h/255.0 ) ), Scalar( 0, 0, 255 ), -1 );
}
*/
///define beginning search window for camshift 


///employ camshift algorithm
RotatedRect trackBox = CamShift(backproj, trackWindow, TermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ));
//imshow( "Hist", histImg );
//imshow( "Histnomask", hist_nomask_img );
///draw shape from camshift
Point2f vertices[4];
trackBox.points(vertices);
rectangle(image,trackWindow, Scalar(0,255,0),1,8,0);
for (int i = 0; i < 4; i++){
line(image, vertices[i], vertices[(i+1)%4], Scalar(0,255,0));
}
center = trackBox.center;

if((center.x != 160) && (center.y !=120))
{
circle(image, center, 20, Scalar(255,255,255), 3,8,0);
x_mbed=center.x;
xcoord=x_mbed+100;
y_mbed=center.y;
ycoord=y_mbed+100;
p1= Point(xcoord, ycoord);
trackWindow = trackBox.boundingRect();
cout << "(x,y)=" << center << "\n";
}
else
{
cout << "nothing found" << "\n";
xcoord=555;
ycoord=555;

rows = backproj.rows;
cols = backproj.cols;
sizex=cols/2;
sizey=rows/2;
p1= Point(cols/2-sizex/2,rows/2-sizey/2);
box = Size(sizex, sizey);
trackWindow=Rect(p1, box);
}

//imshow( "Back Proj", backproj );
//imshow( "Histogram no Mask", hist_nomask_img);
}
void Mask_Calc(int, void*){

//cvtColor(ref, ref_hsv, CV_BGR2HSV);	
//imshow("", ref_hsv);		
mask=Mat::zeros(ref_hsv.size(), ref_hsv.type());
//inRange(ref_hsv, Scalar(0,smin,vmin,0), Scalar(256, 256, vmax,0), mask);
inRange(ref_hsv, Scalar(0,220,0,0), Scalar(256, 256, 256,0), mask); //red
	
}
/*
char Open( const char* Device,unsigned int Bauds)
{
struct termios options;                                             // Structure with the device's options


    // Open device
    fd = open(Device, O_RDWR | O_NOCTTY | O_NDELAY);                    // Open port
    if (fd == -1) return -2;                                            // If the device is not open, return -1
    fcntl(fd, F_SETFL, FNDELAY);                                        // Open the device in nonblocking mode

    // Set parameters
    tcgetattr(fd, &options);                                            // Get the current options of the port
    bzero(&options, sizeof(options));                                   // Clear all the options
    speed_t         Speed;
    switch (Bauds)                                                      // Set the speed (Bauds)
    {
    case 110  :     Speed=B110; break;
    case 300  :     Speed=B300; break;
    case 600  :     Speed=B600; break;
    case 1200 :     Speed=B1200; break;
    case 2400 :     Speed=B2400; break;
    case 4800 :     Speed=B4800; break;
    case 9600 :     Speed=B9600; break;
    case 19200 :    Speed=B19200; break;
    case 38400 :    Speed=B38400; break;
    case 57600 :    Speed=B57600; break;
    case 115200 :   Speed=B115200; break;
    default : return -4;
}
    cfsetispeed(&options, Speed);                                       // Set the baud rate at 115200 bauds
    cfsetospeed(&options, Speed);
    options.c_cflag |= ( CLOCAL | CREAD |  CS8);                        // Configure the device : 8 bits, no parity, no control
    options.c_iflag |= ( IGNPAR | IGNBRK );
    options.c_cc[VTIME]=0;                                              // Timer unused
    options.c_cc[VMIN]=0;                                               // At least on character before satisfy reading
    tcsetattr(fd, TCSANOW, &options);                                   // Activate the settings
    return (1);                                         
}
*/
/*
void Close()
{
    close (fd);

}

char WriteString (const char* String)
{

    int Lenght=strlen(String);                                          // Lenght of the string
    if (write(fd,String,Lenght)!=Lenght)                                // Write the string
        return -1;                                                      // error while writing
    return 1;                                                           // Write operation successfull

}

char ReadChar(char *pByte)
{

        switch (read(fd,pByte,1)) {                                     // Try to read a byte on the device
        case 1  : return 1;                                             // Read successfull
        case -1 : return -2;                                            // Error while reading
        }
    
    return 0;

}


int ReadString(char *String,char FinalChar,unsigned int MaxNbBytes)
{
    unsigned int    NbBytes=0;                                          // Number of bytes read
    char            ret;                                                // Returned value from Read
    while (NbBytes<MaxNbBytes)                                          // While the buffer is not full
    {                                                                   // Read a byte with the restant time
        ret=ReadChar(&String[NbBytes]);
        if (ret==1)                                                     // If a byte has been read
        {
            if (String[NbBytes]==FinalChar)                             // Check if it is the final char
            {
                String  [++NbBytes]=0;                                  // Yes : add the end character 0
                return NbBytes;                                         // Return the number of bytes read
            }
            NbBytes++;                                                  // If not, just increase the number of bytes read
        }
        if (ret<0) return ret;                                          // Error while reading : return the error number
    }
    return -3;                                                          // Buffer is full : return -3
}


void FlushReceiver()
{

    tcflush(fd,TCIFLUSH);

}

*/
