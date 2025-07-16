// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#ifndef YOLO_H
#define YOLO_H

#include <opencv2/core/core.hpp>

#include <net.h>
#include <chrono>

struct Object
{
    cv::Rect_<float> rect;
    int label;
    float prob;
};
struct GridAndStride
{
    int grid0;
    int grid1;
    int stride;
};
class Yolo
{
public:
    // 定义检测模式常量
    static const int DETECT_MODE_HUMAN_ONLY = 0;
    static const int DETECT_MODE_HUMAN_AND_VEHICLE = 1;
    
    Yolo();

    int load(const char* modeltype, int target_size, const float* mean_vals, const float* norm_vals, bool use_gpu = false);

    int load(AAssetManager* mgr, const char* modeltype, int target_size, const float* mean_vals, const float* norm_vals, bool use_gpu = false);

    // detect返回1表示被节流，0表示正常推理
    int detect(const cv::Mat& rgb, std::vector<Object>& objects, float prob_threshold = 0.4f, float nms_threshold = 0.5f);

    int draw(cv::Mat& rgb, const std::vector<Object>& objects);
    
    // 设置推理节流间隔（毫秒）
    void setThrottleInterval(int interval);
    
    // 获取当前节流间隔
    int getThrottleInterval();
    
    // 判断是否应该跳过推理
    bool shouldSkipInference();
    
    // 设置检测模式
    void setDetectMode(int mode);
    
    // 获取当前检测模式
    int getDetectMode();

private:
    ncnn::Net yolo;
    int target_size;
    float mean_vals[3];
    float norm_vals[3];
    ncnn::UnlockedPoolAllocator blob_pool_allocator;
    ncnn::PoolAllocator workspace_pool_allocator;
    
    // 节流控制
    int throttle_interval; // 以毫秒为单位
    std::chrono::steady_clock::time_point last_inference_time;
    
    // 检测模式
    int detect_mode;
};

#endif // NANODET_H
