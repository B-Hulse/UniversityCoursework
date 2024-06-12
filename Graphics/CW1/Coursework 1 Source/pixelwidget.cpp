#include <QtGui>
#include <QApplication>
#include <QMainWindow>
#include <QMessageBox>
#include <QPainter>
#include <QLabel>
#include <QDesktopWidget>
#include <iostream>
#include <fstream>

#include "pixelwidget.hpp"

#include <stdlib.h>

void PixelWidget::DefinePixelValues()
{ //add pixels here; write methods like this for the assignments
  SetPixel(69, 69, RGBVal(255, 0, 0));
}

void PixelWidget::DrawLine(float x1, float y1, float x2, float y2, RGBVal rgb1, RGBVal rgb2)
{
  // Line = (1.0 - t)(x1,y1) + t(x2,y2)

  // The step count will be the greatest of the distance between the x or y coordinates
  double stepCount = (std::max(fabs(x1 - x2), fabs(y1 - y2)));

  // For a range of values of t given by stepCount
  for (double t = 0; t <= 1; t += (1.0 / stepCount))
  {
    // Interpolate the perameterized line equaion with the value of t
    int xt = round(((1.0 - t) * x1) + (t * x2));
    int yt = round(((1.0 - t) * y1) + (t * y2));

    // Interpolate the colour like it were a perameterised line in 3D
    int rt = round(((1.0 - t) * rgb1._red) + (t * rgb2._red));
    int gt = round(((1.0 - t) * rgb1._green) + (t * rgb2._green));
    int bt = round(((1.0 - t) * rgb1._blue) + (t * rgb2._blue));

    // Set the pixel for that value of t
    SetPixel(xt, yt, RGBVal(rt, gt, bt));
  }
}

void PixelWidget::DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                               RGBVal rgb1, RGBVal rgb2, RGBVal rgb3)
{
  // Triangle = s(x1,y1) + t(x2,y2) + (1-t-s)(x3,y3) : Where 0 <= s + t <= 1

  // Find the width and height of the triangle
  int max_x = std::max(std::max(abs(x1-x2),abs(x1-x3)),abs(x2-x3));
  int max_y = std::max(std::max(abs(y1-y2),abs(y1-y3)),abs(y2-y3));

  // Take the larger of the two and multiple
  double max_side = std::max(max_x,max_y);

  // For a range of values of s and t dictated by the length of the max_side
  for (double s = 0; s <= 1; s += (1.0/max_side)) {
    for (double t = 0; s + t <= 1; t += (1.0/max_side)) {
      // Interpolate the barycentric coordinates
      int xst = round((s*x1) + (t*x2) + ((1-s-t)*x3));
      int yst = round((s*y1) + (t*y2) + ((1-s-t)*y3));

      // Interpolate the colours as if they were given as barycentric coordinates of a plane in 3D
      int rst = round((s*rgb1._red)+(t*rgb2._red)+((1-s-t)*rgb3._red));
      int gst = round((s*rgb1._green)+(t*rgb2._green)+((1-s-t)*rgb3._green));
      int bst = round((s*rgb1._blue)+(t*rgb2._blue)+((1-s-t)*rgb3._blue));

      SetPixel(xst,yst,RGBVal(rst,gst,bst));
    }
  }
}

// (x1,y1), (x2,y2), (x3,y3) must be passed to the function in anti-clockwise order.
bool PixelWidget::IsInside(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
  // Find the Vectors for each edge
  float edge_1_x = x2-x1;
  float edge_1_y = y2-y1;

  float edge_2_x = x3-x2;
  float edge_2_y = y3-y2;

  float edge_3_x = x1-x3;
  float edge_3_y = y1-y3;

  // Find the normals to each edge
  float normal_1_x = edge_1_y;
  float normal_1_y = -edge_1_x;

  float normal_2_x = edge_2_y;
  float normal_2_y = -edge_2_x;

  float normal_3_x = edge_3_y;
  float normal_3_y = -edge_3_x;

  // f(X) = (X-P).n
  // For each edge, find which side of them the point lies
  // side_x > 0 : Above, side_x = 0 : On, side_x < 0 : Below
  float side_1 = ((x4-x1)*normal_1_x) + ((y4-y1)*normal_1_y);
  float side_2 = ((x4-x2)*normal_2_x) + ((y4-y2)*normal_2_y);
  float side_3 = ((x4-x3)*normal_3_x) + ((y4-y3)*normal_3_y);

  // If the point is above or on all three lines
  if (side_1 >= 0 && side_2 >= 0 && side_3 >= 0) {
    return true;
  } else {
    return false;
  }
}

void PixelWidget::CheckTriangle(float x1, float y1, float x2, float y2, float x3, float y3) {
  std::ofstream outFile;
  outFile.open("output.txt");

  
  // Find the determinent of T
  float det = ((x1-x3)*(y2-y3))-((x2-x3)*(y1-y3));

  // Define the components of T^-1
  // invTxy is the value at (x,y) in the matric T^-1
  float invT00 = (y2-y3) / det;
  float invT10 = (x3-x2) / det;
  float invT01 = (y3-y1) / det;
  float invT11 = (x1-x3) / det;

  // For each pixel (x,y) in the canvas
  for (unsigned int x = 0; x < _n_vertical; x++) {
    for (unsigned int y = 0; y < _n_horizontal; y++) {
      // cx is an intermediary value in calculateing the baryxentric coordinates of the point
      float c1 = x - x3;
      float c2 = y - y3;

      // Find the barycentric coordinates of the point
      float alpha = (c1 * invT00) + (c2 * invT10);
      float beta = (c1 * invT01) + (c2 * invT11);

      // Test if the point lies in the triangle
      bool isIn = IsInside(x1,y1,x2,y2,x3,y3,x,y);

      // Write to the output File
      outFile << (isIn ? "True" : "False") << ":" << alpha << ":" << beta << "\n";
    }
  }
  outFile.close();
}

// Function to output the current viewport as an image file in the ppm format with the name 'filename'
void PixelWidget::OutputPPM(const char* filename) {
  // Initialise and load the file with the name given by filename
  std::ofstream outFile;
  try
  {
    outFile.open(filename);
  }
  catch(const std::exception& e)
  {
    std::cerr << "Error opening file. \n";
  }
  
  // Write the header of the ppm
  outFile << "P3 " << _n_horizontal << " " << _n_vertical << " 255\n";

  // For each pixel in the image
  for (unsigned int y = 0; y < _n_horizontal; y++) {
    for (unsigned int x = 0; x < _n_vertical; x++) {
      // Get the colour of the pixel at the coordinates (x,y)
      RGBVal col = _vec_rects[x][y];
      // Write to the file
      outFile << col._red << " " << col._green << " " << col._blue << " ";
    }
    outFile << "\n";
  }

  // Clean up the outFile object
  outFile.close();

}


// -----------------Most code below can remain untouched -------------------------
// ------but look at where DefinePixelValues is called in paintEvent--------------

PixelWidget::PixelWidget(unsigned int n_vertical, unsigned int n_horizontal) : _n_vertical(n_vertical),
                                                                               _n_horizontal(n_horizontal),
                                                                               _vec_rects(0)
{
  // all pixels are initialized to black
  for (unsigned int i_col = 0; i_col < n_vertical; i_col++)
    _vec_rects.push_back(std::vector<RGBVal>(n_horizontal));
}

void PixelWidget::SetPixel(unsigned int i_column, unsigned int i_row, const RGBVal &rgb)
{

  // if the pixel exists, set it
  if (i_column < _n_vertical && i_row < _n_horizontal)
    _vec_rects[i_column][i_row] = rgb;
}

void PixelWidget::paintEvent(QPaintEvent *)
{

  QPainter p(this);
  // no antialiasing, want thin lines between the cell
  p.setRenderHint(QPainter::Antialiasing, false);

  // set window/viewport so that the size fits the screen, within reason
  p.setWindow(QRect(-1., -1., _n_vertical + 2, _n_horizontal + 2));
  int side = qMin(width(), height());
  p.setViewport(0, 0, side, side);

  // black thin boundary around the cells
  QPen pen(Qt::black);
  pen.setWidth(0.);

  // here the pixel values defined by the user are set in the pixel array
  DefinePixelValues();

  DrawLine(12, 63, 54, 25, RGBVal(255, 0, 0), RGBVal(0, 255, 0));
  DrawLine(0, 0, 69, 69, RGBVal(125,10,236), RGBVal(255, 255, 255));
  DrawLine(10, 0.0, 10, 68, RGBVal(125, 10, 236), RGBVal(255, 255, 255));
  DrawLine(70, 10, 0, 10, RGBVal(125, 10, 236), RGBVal(255, 255, 255));
  DrawLine(70, 70, 0, 0, RGBVal(125, 10, 236), RGBVal(255, 255, 255));

  DrawTriangle(11,31,57,67,69,11,RGBVal(255,0,0),RGBVal(0,255,0),RGBVal(0,0,255));
  CheckTriangle(11,31,57,67,69,11);
  OutputPPM("out.ppm");

  for (unsigned int i_column = 0; i_column < _n_vertical; i_column++)
    for (unsigned int i_row = 0; i_row < _n_horizontal; i_row++)
    {
      QRect rect(i_column, i_row, 1, 1);
      QColor c = QColor(_vec_rects[i_column][i_row]._red, _vec_rects[i_column][i_row]._green, _vec_rects[i_column][i_row]._blue);

      // fill with color for the pixel
      p.fillRect(rect, QBrush(c));
      p.setPen(pen);
      p.drawRect(rect);
    }
}
