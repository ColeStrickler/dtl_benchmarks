#ifndef BIJECTIVE_HPP
#define BIJECTIVE_HPP

#include <vector>
#include <cassert>
#include <stdint.h>
#include <algorithm>





class Shape
{
public:
    Shape(const std::vector<int>& dim);
    ~Shape();
    Shape(const Shape&) = default;
    Shape& operator=(const Shape&) = default;
    std::vector<int> GetStrides();
    void SetStrides();
    void Rotate2D();
    void Rotate3D_Front();
    void Rotate3D_Side();
    void nhwc_to_nchw();
    inline uint32_t LinearIndex(const std::vector<int>& coord) const {
        uint32_t idx = 0;
        for (int i = coord.size()-1; i >= 0; i--)
        {
            idx  += coord[i]*m_Strides[i];
        }

        return idx;
    }

    int GetDimensionality() const {return m_Dim.size();}
    std::vector<int> GetDimensions() {return m_Dim;}
private:
    std::vector<int> m_Dim;
    std::vector<int> m_Strides;
};




Shape permute_rot_left2D(float* in, float* out, Shape shape_in);
Shape permute_rot_right2D(float* in, float* out, Shape shape_in);
Shape permute_reflectx_rot_left2D(float* in, float* out, Shape shape_in);
Shape permute_reflectx_rot_right2D(float* in, float* out, Shape shape_in);
Shape permute_reflect_x2D(float* in, float* out, Shape shape_in);
Shape permute_reflect_y2D(float* in, float* out, Shape shape_in);
Shape permute_reflect_y_reflect_x2D(float* in, float* out, Shape shape_in);

Shape fuse_permute_rot_left2D(float* in, float* out, float* out2, Shape shape_in);
Shape fuse_permute_rot_right2D(float* in, float* out,  float* out2, Shape shape_in);
Shape fuse_permute_reflectx_rot_left2D(float* in, float* out,  float* out2, Shape shape_in);
Shape fuse_permute_reflectx_rot_right2D(float* in, float* out, float* out2, Shape shape_in);
Shape fuse_permute_reflect_x2D(float* in, float* out,  float* out2, Shape shape_in);
Shape fuse_permute_reflect_y2D(float* in, float* out,  float* out2, Shape shape_in);
Shape fuse_permute_reflect_y_reflect_x2D(float* in, float* out,  float* out2, Shape shape_in);







void ConvBatch(float* filter, float* in, float* out, Shape shape_in);
void IntensityScaleBatch(float scalar, float *in, float* out, Shape shape_in);



#endif