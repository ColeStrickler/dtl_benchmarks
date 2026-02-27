#include "bijective.hpp"



/*
    We modify in place
*/
void IntensityScaleBatch(float scalar, float *in, float* out, Shape shape_in)
{

    //shape in is in nchw
    auto dim = shape_in.GetDimensions();
    int n = dim[0];
    int c = dim[1];
    int h = dim[2];
    int w = dim[3];
    int i = 0;
    for (int j = 0; j < n*c*h*w; j++)
        out[j] = scalar+in[j];
        
}






Shape permute_rot_left2D(float *in, float *out, Shape shape_in) {
  assert(shape_in.GetDimensionality() == 4);
  // we take in a NHWC
  // We need to translate from this to NCWH
  /*
      We can iterate the coordinates the same as in the DTU
  */
  Shape outshape = shape_in;
  outshape.nhwc_to_nchw();
  outshape.Rotate2D();
  shape_in.SetStrides();

  std::vector<int> out_dim = outshape.GetDimensions();
  std::vector<int> in_dim = shape_in.GetDimensions();
  int n = out_dim[0];
  int c = out_dim[1];
  int w = out_dim[2];
  int h = out_dim[3];

  assert(n == in_dim[0]);
  assert(h == in_dim[1]);
  assert(w == in_dim[2]);
  assert(c == in_dim[3]);

  // With rotate left, out(y, x) = in(x, Y - 1 - y)
  int W = w;
  int H = h;
  int i = 0;
  for (int nx = 0; nx < n; nx++) {
    for (int cx = 0; cx < c; cx++) {
      for (int wx = 0; wx < w; wx++) {
        for (int hx = 0; hx < h; hx++) {
          out[i++] = in[shape_in.LinearIndex({nx, hx, W - 1 - wx, cx})]; //  is inlined
        }
      }
    }
  }

  return outshape;
}


Shape fuse_permute_rot_left2D(float *in,
                              float *out,
                              float *out2,
                              Shape shape_in)
{
  assert(shape_in.GetDimensionality() == 4);
  // we take in a NHWC
  // We need to translate from this to NCWH
  /*
      We can iterate the coordinates the same as in the DTU
  */
  Shape outshape = shape_in;
  outshape.nhwc_to_nchw();
  outshape.Rotate2D();
  shape_in.SetStrides();

  std::vector<int> out_dim = outshape.GetDimensions();
  std::vector<int> in_dim = shape_in.GetDimensions();
  int n = out_dim[0];
  int c = out_dim[1];
  int w = out_dim[2];
  int h = out_dim[3];

  assert(n == in_dim[0]);
  assert(h == in_dim[1]);
  assert(w == in_dim[2]);
  assert(c == in_dim[3]);

  // With rotate left, out(y, x) = in(x, Y - 1 - y)
  int W = w;
  int H = h;
  int i = 0;
  int j = 0;
  for (int nx = 0; nx < n; nx++) {
    for (int cx = 0; cx < c; cx++) {
      for (int wx = 0; wx < w; wx++) {
        for (int hx = 0; hx < h; hx++) {
            int idx = shape_in.LinearIndex({nx, hx, wx, cx}); //  is inlined
            out[i] = in[idx];
            out2[i++] = 0.1f + in[idx];
        }
      }
    }
  }

  return outshape;
}

Shape fuse_permute_rot_right2D(float *in,
                               float *out,
                               float *out2,
                               Shape shape_in) {
    assert(shape_in.GetDimensionality() == 4);
    // we take in a NHWC
    // We need to translate from this to NCWH
    /*
        We can iterate the coordinates the same as in the DTU
    */
    Shape outshape = shape_in;
    outshape.nhwc_to_nchw();
    outshape.Rotate2D();
    shape_in.SetStrides();
    std::vector<int> out_dim = outshape.GetDimensions();
    std::vector<int> in_dim = shape_in.GetDimensions();
    int n = out_dim[0];
    int c = out_dim[1];
    int w = out_dim[2];
    int h = out_dim[3];


    assert(n == in_dim[0]);
    assert(h == in_dim[1]);
    assert(w == in_dim[2]);
    assert(c == in_dim[3]);

    // With rotate right, out(y, x) = in(H - 1 - x, y)
    int W = w;
    int H = h;
    int i = 0;
    for (int nx = 0; nx < n; nx++)
    {
        for (int cx = 0; cx < c; cx++)
        {
            for (int wx = 0; wx < w; wx++)
            {
                for (int hx = 0; hx < h; hx++)
                {
                    int idx = shape_in.LinearIndex({nx, hx, wx, cx}); //  is inlined
                    out[i] = in[idx];
                    out2[i++] = 0.1f + in[idx];
                }
            }
        }
    }

    return outshape;
}



Shape permute_rot_right2D(float *in, float *out, Shape shape_in)
{
    assert(shape_in.GetDimensionality() == 4);
    // we take in a NHWC
    // We need to translate from this to NCWH
    /*
        We can iterate the coordinates the same as in the DTU
    */
    Shape outshape = shape_in;
    outshape.nhwc_to_nchw();
    outshape.Rotate2D();
    shape_in.SetStrides();
    std::vector<int> out_dim = outshape.GetDimensions();
    std::vector<int> in_dim = shape_in.GetDimensions();
    int n = out_dim[0];
    int c = out_dim[1];
    int w = out_dim[2];
    int h = out_dim[3];


    assert(n == in_dim[0]);
    assert(h == in_dim[1]);
    assert(w == in_dim[2]);
    assert(c == in_dim[3]);

    // With rotate right, out(y, x) = in(H - 1 - x, y)
    int W = w;
    int H = h;
    int i = 0;
    for (int nx = 0; nx < n; nx++)
    {
        for (int cx = 0; cx < c; cx++)
        {
            for (int wx = 0; wx < w; wx++)
            {
                for (int hx = 0; hx < h; hx++)
                {
                    out[i++] = in[shape_in.LinearIndex({nx, H-1-hx, wx, cx})]; //  is inlined
                }
            }
        }
    }

    return outshape;
}
Shape fuse_permute_reflectx_rot_left2D(float *in,
                                       float *out,
                                       float *out2,
                                       Shape shape_in) {
    assert(shape_in.GetDimensionality() == 4);
    // we take in a NHWC
    // We need to translate from this to NCWH
    /*
        We can iterate the coordinates the same as in the DTU
    */
    Shape outshape = shape_in;
    outshape.nhwc_to_nchw();
    outshape.Rotate2D();
    shape_in.SetStrides();

    std::vector<int> out_dim = outshape.GetDimensions();
    std::vector<int> in_dim = shape_in.GetDimensions();
    int n = out_dim[0];
    int c = out_dim[1];
    int w = out_dim[2];
    int h = out_dim[3];

    
    assert(n == in_dim[0]);
    assert(h == in_dim[1]);
    assert(w == in_dim[2]);
    assert(c == in_dim[3]);
    

    // REFLECT: (h1,w1) = (h1, W-1-w1)
    // ROTLEFT: # i,j --> [j, H - 1 - i] --> (h, W-1-(W-1-w1))
    int W = w;
    int H = h;
    int i = 0;
    for (int nx = 0; nx < n; nx++)
    {
        for (int cx = 0; cx < c; cx++)
        {
            for (int wx = 0; wx < w; wx++)
            {
                for (int hx = 0; hx < h; hx++)
                {
                    int idx = shape_in.LinearIndex({nx, hx, wx, cx}); //  is inlined
                    out[i] = in[idx];
                    out2[i++] = 0.1f + in[idx];
                } 
            }
        }
    }

    return outshape;
}

Shape permute_reflectx_rot_left2D(float *in, float *out, Shape shape_in) {
  assert(shape_in.GetDimensionality() == 4);
  // we take in a NHWC
  // We need to translate from this to NCWH
  /*
      We can iterate the coordinates the same as in the DTU
  */
  Shape outshape = shape_in;
  outshape.nhwc_to_nchw();
  outshape.Rotate2D();
  shape_in.SetStrides();

  std::vector<int> out_dim = outshape.GetDimensions();
  std::vector<int> in_dim = shape_in.GetDimensions();
  int n = out_dim[0];
  int c = out_dim[1];
  int w = out_dim[2];
  int h = out_dim[3];

  assert(n == in_dim[0]);
  assert(h == in_dim[1]);
  assert(w == in_dim[2]);
  assert(c == in_dim[3]);

  // REFLECT: (h1,w1) = (h1, W-1-w1)
  // ROTLEFT: # i,j --> [j, H - 1 - i] --> (h, W-1-(W-1-w1))
  int W = w;
  int H = h;
  int i = 0;
  for (int nx = 0; nx < n; nx++) {
    for (int cx = 0; cx < c; cx++) {
      for (int wx = 0; wx < w; wx++) {
        for (int hx = 0; hx < h; hx++) {
          out[i++] = in[shape_in.LinearIndex({nx, hx, wx, cx})];  //  is inlined
        }
      }
    }
  }

  return outshape;
}

Shape fuse_permute_reflectx_rot_right2D(float *in,
                                        float *out,
                                        float *out2,
                                        Shape shape_in) {
    assert(shape_in.GetDimensionality() == 4);
    // we take in a NHWC
    // We need to translate from this to NCWH
    /*
        We can iterate the coordinates the same as in the DTU
    */
    Shape outshape = shape_in;
    outshape.nhwc_to_nchw();
    outshape.Rotate2D();
    shape_in.SetStrides();

    std::vector<int> out_dim = outshape.GetDimensions();
    std::vector<int> in_dim = shape_in.GetDimensions();
    int n = out_dim[0];
    int c = out_dim[1];
    int w = out_dim[2];
    int h = out_dim[3];

    
    assert(n == in_dim[0]);
    assert(h == in_dim[1]);
    assert(w == in_dim[2]);
    assert(c == in_dim[3]);
    

    // REFLECT: (h1,w1) = (h1, W-1-w1)
    // ROTRIGHT: # i,j --> (i,j) -->  [W - 1 - j, i]  --> (H-1-h, W-1-w)
    int W = w;
    int H = h;
    int i = 0;
    for (int nx = 0; nx < n; nx++)
    {
        for (int cx = 0; cx < c; cx++)
        {
            for (int wx = 0; wx < w; wx++)
            {
                for (int hx = 0; hx < h; hx++)
                {
                    int idx = shape_in.LinearIndex({nx, hx, wx, cx}); //  is inlined
                    out[i] = in[idx];
                    out2[i++] = 0.1f + in[idx];
                } 
            }
        }
    }

    return outshape;
}


Shape permute_reflectx_rot_right2D(float *in, float *out, Shape shape_in)
{
    assert(shape_in.GetDimensionality() == 4);
    // we take in a NHWC
    // We need to translate from this to NCWH
    /*
        We can iterate the coordinates the same as in the DTU
    */
    Shape outshape = shape_in;
    outshape.nhwc_to_nchw();
    outshape.Rotate2D();
    shape_in.SetStrides();

    std::vector<int> out_dim = outshape.GetDimensions();
    std::vector<int> in_dim = shape_in.GetDimensions();
    int n = out_dim[0];
    int c = out_dim[1];
    int w = out_dim[2];
    int h = out_dim[3];

    
    assert(n == in_dim[0]);
    assert(h == in_dim[1]);
    assert(w == in_dim[2]);
    assert(c == in_dim[3]);
    

    // REFLECT: (h1,w1) = (h1, W-1-w1)
    // ROTRIGHT: # i,j --> (i,j) -->  [W - 1 - j, i]  --> (H-1-h, W-1-w)
    int W = w;
    int H = h;
    int i = 0;
    for (int nx = 0; nx < n; nx++)
    {
        for (int cx = 0; cx < c; cx++)
        {
            for (int wx = 0; wx < w; wx++)
            {
                for (int hx = 0; hx < h; hx++)
                {
                    out[i++] = in[shape_in.LinearIndex({nx, H-1-hx, W-1-wx, cx})]; //  is inlined
                } 
            }
        }
    }

    return outshape;
}



Shape fuse_permute_reflect_x2D(float *in,
                               float *out,
                               float *out2,
                               Shape shape_in) {
    assert(shape_in.GetDimensionality() == 4);
    // we take in a NHWC
    // We need to translate from this to NCWH
    /*
        We can iterate the coordinates the same as in the DTU
    */
    Shape outshape = shape_in;
    outshape.nhwc_to_nchw();
    shape_in.SetStrides();
    shape_in.SetStrides();

    std::vector<int> out_dim = outshape.GetDimensions();
    std::vector<int> in_dim = shape_in.GetDimensions();
    int n = out_dim[0];
    int c = out_dim[1];
    int h = out_dim[2];
    int w = out_dim[3];


    
    assert(n == in_dim[0]);
    assert(h == in_dim[1]);
    assert(w == in_dim[2]);
    assert(c == in_dim[3]);
    


    int W = w;
    int H = h;
    int i = 0;
    for (int nx = 0; nx < n; nx++)
    {
        for (int cx = 0; cx < c; cx++)
        {
            for (int hx = 0; hx < h; hx++)
            {
                for (int wx = 0; wx < w; wx++)
                {
                    int idx = shape_in.LinearIndex({nx, hx, wx, cx}); //  is inlined
                    out[i] = in[idx];
                    out2[i++] = 0.1f + in[idx];
                } 
            }
        }
    }

    return outshape;
}



Shape permute_reflect_x2D(float *in, float *out, Shape shape_in) 
{
    assert(shape_in.GetDimensionality() == 4);
    // we take in a NHWC
    // We need to translate from this to NCWH
    /*
        We can iterate the coordinates the same as in the DTU
    */
    Shape outshape = shape_in;
    outshape.nhwc_to_nchw();
    shape_in.SetStrides();
    shape_in.SetStrides();

    std::vector<int> out_dim = outshape.GetDimensions();
    std::vector<int> in_dim = shape_in.GetDimensions();
    int n = out_dim[0];
    int c = out_dim[1];
    int h = out_dim[2];
    int w = out_dim[3];


    
    assert(n == in_dim[0]);
    assert(h == in_dim[1]);
    assert(w == in_dim[2]);
    assert(c == in_dim[3]);
    


    int W = w;
    int H = h;
    int i = 0;
    for (int nx = 0; nx < n; nx++)
    {
        for (int cx = 0; cx < c; cx++)
        {
            for (int hx = 0; hx < h; hx++)
            {
                for (int wx = 0; wx < w; wx++)
                {
                    out[i++] = in[shape_in.LinearIndex({nx, hx, W-1-wx, cx})]; //  is inlined
                } 
            }
        }
    }

    return outshape;
}



Shape fuse_permute_reflect_y2D(float *in,
                               float *out,
                               float *out2,
                               Shape shape_in) {
    assert(shape_in.GetDimensionality() == 4);
    // we take in a NHWC
    // We need to translate from this to NCWH
    /*
        We can iterate the coordinates the same as in the DTU
    */
    Shape outshape = shape_in;
    outshape.nhwc_to_nchw();
    shape_in.SetStrides();

    std::vector<int> out_dim = outshape.GetDimensions();
    std::vector<int> in_dim = shape_in.GetDimensions();
    int n = out_dim[0];
    int c = out_dim[1];
    int h = out_dim[2];
    int w = out_dim[3];


    
    assert(n == in_dim[0]);
    assert(h == in_dim[1]);
    assert(w == in_dim[2]);
    assert(c == in_dim[3]);
    

    int W = w;
    int H = h;
    int i = 0;
    for (int nx = 0; nx < n; nx++)
    {
        for (int cx = 0; cx < c; cx++)
        {
            for (int hx = 0; hx < h; hx++)
            {
                for (int wx = 0; wx < w; wx++)
                {
                    int idx = shape_in.LinearIndex({nx, hx, wx, cx}); //  is inlined
                    out[i] = in[idx];
                    out2[i++] = 0.1f + in[idx];
                } 
            }
        }
    }

    return outshape;
}



Shape permute_reflect_y2D(float *in, float *out, Shape shape_in) 
{
    assert(shape_in.GetDimensionality() == 4);
    // we take in a NHWC
    // We need to translate from this to NCWH
    /*
        We can iterate the coordinates the same as in the DTU
    */
    Shape outshape = shape_in;
    outshape.nhwc_to_nchw();
    shape_in.SetStrides();

    std::vector<int> out_dim = outshape.GetDimensions();
    std::vector<int> in_dim = shape_in.GetDimensions();
    int n = out_dim[0];
    int c = out_dim[1];
    int h = out_dim[2];
    int w = out_dim[3];


    
    assert(n == in_dim[0]);
    assert(h == in_dim[1]);
    assert(w == in_dim[2]);
    assert(c == in_dim[3]);
    

    int W = w;
    int H = h;
    int i = 0;
    for (int nx = 0; nx < n; nx++)
    {
        for (int cx = 0; cx < c; cx++)
        {
            for (int hx = 0; hx < h; hx++)
            {
                for (int wx = 0; wx < w; wx++)
                {
                    out[i++] = in[shape_in.LinearIndex({nx, H-1-hx, wx, cx})]; //  is inlined
                } 
            }
        }
    }

    return outshape;
}



Shape fuse_permute_reflect_y_reflect_x2D(float *in,
                                         float *out,
                                         float *out2,
                                         Shape shape_in) {
    assert(shape_in.GetDimensionality() == 4);
    // we take in a NHWC
    // We need to translate from this to NCWH
    /*
        We can iterate the coordinates the same as in the DTU
    */
    Shape outshape = shape_in;
    outshape.nhwc_to_nchw();
    shape_in.SetStrides();


    std::vector<int> out_dim = outshape.GetDimensions();
    std::vector<int> in_dim = shape_in.GetDimensions();
    int n = out_dim[0];
    int c = out_dim[1];
    int h = out_dim[2];
    int w = out_dim[3];


    
    assert(n == in_dim[0]);
    assert(h == in_dim[1]);
    assert(w == in_dim[2]);
    assert(c == in_dim[3]);
    

    int W = w;
    int H = h;
    int i = 0;
    for (int nx = 0; nx < n; nx++)
    {
        for (int cx = 0; cx < c; cx++)
        {
            for (int hx = 0; hx < h; hx++)
            {
                for (int wx = 0; wx < w; wx++)
                {
                    int idx = shape_in.LinearIndex({nx, hx, wx, cx}); //  is inlined
                    out[i] = in[idx];
                    out2[i++] = 0.1f + in[idx];
                } 
            }
        }
    }

    return outshape;
}

Shape permute_reflect_y_reflect_x2D(float *in, float *out, Shape shape_in) 
{
    assert(shape_in.GetDimensionality() == 4);
    // we take in a NHWC
    // We need to translate from this to NCWH
    /*
        We can iterate the coordinates the same as in the DTU
    */
    Shape outshape = shape_in;
    outshape.nhwc_to_nchw();
    shape_in.SetStrides();


    std::vector<int> out_dim = outshape.GetDimensions();
    std::vector<int> in_dim = shape_in.GetDimensions();
    int n = out_dim[0];
    int c = out_dim[1];
    int h = out_dim[2];
    int w = out_dim[3];

    auto strides = shape_in.GetStrides();
    
    assert(n == in_dim[0]);
    assert(h == in_dim[1]);
    assert(w == in_dim[2]);
    assert(c == in_dim[3]);
    

    int W = w;
    int H = h;
    int i = 0;




    for (int nx = 0; nx < n; nx++)
    {
        for (int cx = 0; cx < c; cx++)
        {
            for (int hx = 0; hx < h; hx++)
            {
                for (int wx = 0; wx < w; wx++)
                {
                    out[i++] = in[shape_in.LinearIndex({nx, H-1-hx, W-1-wx, cx})]; //  is inlined
                } 
            }
        }
    }

    return outshape;
}



Shape::Shape(const std::vector<int> &dim) : m_Dim(dim)
{

}

Shape::~Shape()
{

}

std::vector<int> Shape::GetStrides() 
{
    int stride = 1;
    std::vector<int> ret;
    for (int i =m_Dim.size()-1; i >= 0; i--)
    {
        ret.push_back(stride);
        stride *= m_Dim[i];
    }
    std::reverse(ret.begin(), ret.end());
    return ret;
}

void Shape::SetStrides()
{
    m_Strides = GetStrides();
}

void Shape::Rotate2D() {
  int size = m_Dim.size();
  assert(size >= 2);
  auto d1 = m_Dim[size - 1];
  m_Dim[size - 1] = m_Dim[size - 2];
  m_Dim[size - 2] = d1;
}
/*
    Let d3, d2, d1  = z, y, x, where


    z is depth, y is height, x is width. 

    z becomes new height; x stays width; y is new depth 

*/
void Shape::Rotate3D_Front() 
{
    int size = m_Dim.size();
    assert(size >= 3);
    auto d2 = m_Dim[size-2];
    auto d3 = m_Dim[size-3];
    m_Dim[size-2] = d3;
    m_Dim[size-3] = d2;
}

/*
    Let d3, d2, d1  = z, y, x, where


    z is depth, y is height, x is width. 

    z becomes new width; x becomes new depth; y stays height 

*/
void Shape::Rotate3D_Side()
{
    int size = m_Dim.size();
    assert(size >= 3);
    auto d1 = m_Dim[size-1];
    auto d3 = m_Dim[size-3];
    m_Dim[size-1] = d3;
    m_Dim[size-3] = d1;
}

void Shape::nhwc_to_nchw()
{
    int size = m_Dim.size();
    assert(size >= 4);
    auto c = m_Dim[size-1];
    auto w = m_Dim[size-2];
    auto h = m_Dim[size-3];
    auto n = m_Dim[size-4];
    m_Dim[size-1] = w;
    m_Dim[size-2] = h;
    m_Dim[size-3] = c;
    m_Dim[size-4] = n;
}

