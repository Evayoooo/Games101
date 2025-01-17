#include "Triangle.hpp"
#include "rasterizer.hpp"
#include <eigen3/Eigen/Eigen>
#include <iostream>
#include <opencv2/opencv.hpp>

constexpr double MY_PI = 3.1415926;

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0], 0, 1, 0, -eye_pos[1], 0, 0, 1,
        -eye_pos[2], 0, 0, 0, 1;

    view = translate * view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the model matrix for rotating the triangle around the Z axis.
    // Then return it.
    double alpha = rotation_angle/180.0*MY_PI;
    Eigen::Matrix4f rotation;
    rotation << 
    std::cos(alpha), -std::sin(alpha), 0, 0,
    std::sin(alpha), std::cos(alpha),  0, 0,
    0,          0,           1, 0,
    0,          0,           0, 1;

    model = rotation * model;
 
    return model;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
                                      float zNear, float zFar)
{
    // Students will implement this function

    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the projection matrix for the given parameters.
    // Then return it.

    float fov = eye_fov / 180 * MY_PI;
    float t = std::tan(fov/2) * zNear;
    float r = aspect_ratio * t;
    float b = -t;
    float l = -r;

    Eigen::Matrix4f M_persep;
    M_persep <<
    zNear, 0, 0, 0,
    0, zNear, 0, 0,
    0, 0, zNear+zFar, -zNear*zFar,
    0, 0,     1, 0; 

    Eigen::Matrix4f M_ortho_scale;
    M_ortho_scale <<
    2/(r-l), 0, 0, 0,
    0, 2/(t-b), 0, 0,
    0, 0, 2/(zNear-zFar), 0,
    0, 0, 0,       1;

    Eigen::Matrix4f M_ortho_pos;
    M_ortho_pos <<
    1, 0, 0, -(r+l)/2,
    0, 1, 0, -(t+b)/2,
    0, 0, 1, -(zNear+zFar)/2,
    0, 0, 0,       1;

    //为了使得三角形是正着显示的，这里需要把透视矩阵乘以下面这样的矩阵
    //参考：http://games-cn.org/forums/topic/%e4%bd%9c%e4%b8%9a%e4%b8%89%e7%9a%84%e7%89%9b%e5%80%92%e8%bf%87%e6%9d%a5%e4%ba%86/
    Eigen::Matrix4f Mt;
    Mt << 1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, -1, 0,
        0, 0, 0, 1; 


    projection = M_ortho_scale * M_ortho_pos * M_persep * Mt * projection;

    return projection;
}


Eigen::Matrix4f get_rotation(Vector3f axis, float angel)
{
    // Rodrigues' Rotation Formula
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
    //去掉长度，保留方向
    float norm = sqrt(axis[0]*axis[0]+axis[1]*axis[1]+axis[2]*axis[2]);
    axis[0]/=norm;
    axis[1]/=norm;
    axis[2]/=norm;

    //围绕任意轴旋转的矩阵公式
    float alpha = angel / 180.0 * MY_PI;
    Eigen::Matrix3f N;
    N << 0, -axis[2], axis[1],
        axis[2],  0, -axis[0],
        -axis[1],axis[0],   0;
    
    Eigen::Matrix3f component1 = cos(alpha) * Eigen::Matrix3f::Identity();
    Eigen::Matrix3f component2 = (1 - cos(alpha)) * axis * axis.transpose();
    Eigen::Matrix3f component3 = N * sin(alpha);

    Eigen::Matrix3f m_rotate = component1 + component2 + component3;

    Eigen::Matrix4f m4_rotate = Eigen::Matrix4f::Identity();
    //罗德里格斯公式中的矩阵都是 3x3 的，而代码中要运用次坐标，因此返回的格式是 4x4 的矩阵，所以转化成m4_rotate
    m4_rotate.block(0,0,3,3) = m_rotate;

    model = m4_rotate * model;
    return model;
}


int main(int argc, const char** argv)
{
    float angle = 0;
    bool command_line = false;
    std::string filename = "output.png";

    if (argc >= 3) {
        command_line = true;
        angle = std::stof(argv[2]); // -r by default
        if (argc == 4) {
            filename = std::string(argv[3]);
        }
        else
            return 0;
    }

    rst::rasterizer r(700, 700);

    Eigen::Vector3f eye_pos = {0, 0, 5};

    // Edit begin
    Eigen::Vector3f rotate_axis = {0, 0, 1};
    // Edit end

    std::vector<Eigen::Vector3f> pos{{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}};

    std::vector<Eigen::Vector3i> ind{{0, 1, 2}};

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);

    int key = 0;
    int frame_count = 0;

    if (command_line) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        // Edit begin
        //围绕z轴旋转
        //r.set_model(get_model_matrix(angle));
        //围绕任意轴旋转
        r.set_model(get_rotation(rotate_axis,angle));
        // Edit end

        //r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);

        cv::imwrite(filename, image);

        return 0;
    }

    while (key != 27) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        // Edit begin
        //围绕z轴旋转
        //r.set_model(get_model_matrix(angle));
        //围绕任意轴旋转
        r.set_model(get_rotation(rotate_axis,angle));
        // Edit end

        //r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::imshow("image", image);
        key = cv::waitKey(10);

        std::cout << "frame count: " << frame_count++ << '\n';

        if (key == 'a') {
            angle += 10;
        }
        else if (key == 'd') {
            angle -= 10;
        }
    }

    return 0;
}
