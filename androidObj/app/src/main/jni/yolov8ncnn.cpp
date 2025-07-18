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

#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

#include <android/log.h>

#include <jni.h>

#include <string>
#include <vector>

#include <platform.h>
#include <benchmark.h>

#include "yolo.h"

#include "ndkcamera.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#if __ARM_NEON
#include <arm_neon.h>
#endif // __ARM_NEON

static int draw_unsupported(cv::Mat& rgb)
{
    const char text[] = "unsupported";

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 1.0, 1, &baseLine);

    int y = (rgb.rows - label_size.height) / 2;
    int x = (rgb.cols - label_size.width) / 2;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                    cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0));

    return 0;
}

static int draw_fps(cv::Mat& rgb)
{
    // resolve moving average
    float avg_fps = 0.f;
    {
        static double t0 = 0.f;
        static float fps_history[10] = {0.f};

        double t1 = ncnn::get_current_time();
        if (t0 == 0.f)
        {
            t0 = t1;
            return 0;
        }

        float fps = 1000.f / (t1 - t0);
        t0 = t1;

        for (int i = 9; i >= 1; i--)
        {
            fps_history[i] = fps_history[i - 1];
        }
        fps_history[0] = fps;

        if (fps_history[9] == 0.f)
        {
            return 0;
        }

        for (int i = 0; i < 10; i++)
        {
            avg_fps += fps_history[i];
        }
        avg_fps /= 10.f;
    }

    char text[32];
    sprintf(text, "FPS=%.2f", avg_fps);

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    int y = 0;
    int x = rgb.cols - label_size.width;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                    cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));

    return 0;
}

static Yolo* g_yolo = 0;
static ncnn::Mutex lock;

static float g_prob_threshold = 0.4f;

class MyNdkCamera : public NdkCameraWindow
{
public:
    MyNdkCamera() {
        last_objects.clear();
    }
    virtual void on_image_render(cv::Mat& rgb) const;
private:
    mutable std::vector<Object> last_objects;
};

void MyNdkCamera::on_image_render(cv::Mat& rgb) const
{
    ncnn::MutexLockGuard g(lock);
    if (g_yolo)
    {
        std::vector<Object> objects;
        int detect_ret = g_yolo->detect(rgb, objects, g_prob_threshold);
        if (detect_ret == 1) // 被节流，复用上次结果
        {
            if (!last_objects.empty())
                g_yolo->draw(rgb, last_objects);
        }
        else // 正常推理
        {
            if (!objects.empty())
                last_objects = objects;
            else
                last_objects.clear(); // 没有目标时清空
            g_yolo->draw(rgb, objects);
        }
    }
    else
    {
        draw_unsupported(rgb);
    }
    draw_fps(rgb);
}

static MyNdkCamera* g_camera = 0;

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "JNI_OnLoad");

    g_camera = new MyNdkCamera;

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved)
{
    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "JNI_OnUnload");

    {
        ncnn::MutexLockGuard g(lock);

        delete g_yolo;
        g_yolo = 0;
    }

    delete g_camera;
    g_camera = 0;
}

// public native boolean loadModel(AssetManager mgr, int modelid, int cpugpu);
JNIEXPORT jboolean JNICALL Java_com_tencent_yolov8ncnn_Yolov8Ncnn_loadModel(JNIEnv* env, jobject thiz, jobject assetManager, jint modelid, jint cpugpu)
{
    if (modelid < 0 || modelid > 6 || cpugpu < 0 || cpugpu > 1)
    {
        return JNI_FALSE;
    }

    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);

    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "loadModel %p", mgr);

    const char* modeltypes[] =
    {
        "n",
        "s",
    };

    const int target_sizes[] =
    {
        320,
        320,
    };

    const float mean_vals[][3] =
    {
        {103.53f, 116.28f, 123.675f},
        {103.53f, 116.28f, 123.675f},
    };

    const float norm_vals[][3] =
    {
        { 1 / 255.f, 1 / 255.f, 1 / 255.f },
        { 1 / 255.f, 1 / 255.f, 1 / 255.f },
    };

    const char* modeltype = modeltypes[(int)modelid];
    int target_size = target_sizes[(int)modelid];
    bool use_gpu = (int)cpugpu == 1;

    // reload
    {
        ncnn::MutexLockGuard g(lock);

        if (use_gpu && ncnn::get_gpu_count() == 0)
        {
            // no gpu
            delete g_yolo;
            g_yolo = 0;
        }
        else
        {
            if (!g_yolo)
                g_yolo = new Yolo;
            g_yolo->load(mgr, modeltype, target_size, mean_vals[(int)modelid], norm_vals[(int)modelid], use_gpu);
        }
    }

    return JNI_TRUE;
}

// public native boolean openCamera(int facing);
JNIEXPORT jboolean JNICALL Java_com_tencent_yolov8ncnn_Yolov8Ncnn_openCamera(JNIEnv* env, jobject thiz, jint facing)
{
    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "openCamera %d", facing);

    int ret = g_camera->open((int)facing);
    
    return ret == 0 ? JNI_TRUE : JNI_FALSE;
}

// public native boolean closeCamera();
JNIEXPORT jboolean JNICALL Java_com_tencent_yolov8ncnn_Yolov8Ncnn_closeCamera(JNIEnv* env, jobject thiz)
{
    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "closeCamera");

    g_camera->close();

    return JNI_TRUE;
}

// public native boolean setOutputWindow(Surface surface);
JNIEXPORT jboolean JNICALL Java_com_tencent_yolov8ncnn_Yolov8Ncnn_setOutputWindow(JNIEnv* env, jobject thiz, jobject surface)
{
    ANativeWindow* win = ANativeWindow_fromSurface(env, surface);

    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "setOutputWindow %p", win);

    g_camera->set_window(win);

    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_tencent_yolov8ncnn_Yolov8Ncnn_setConfidenceThreshold(JNIEnv* env, jobject thiz, jfloat threshold)
{
    g_prob_threshold = threshold;
}

// 设置节流间隔
JNIEXPORT void JNICALL Java_com_tencent_yolov8ncnn_Yolov8Ncnn_setThrottleInterval(JNIEnv* env, jobject thiz, jint milliseconds)
{
    ncnn::MutexLockGuard g(lock);
    
    if (g_yolo)
    {
        g_yolo->setThrottleInterval(milliseconds);
        __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "setThrottleInterval %d ms", milliseconds);
    }
}

// 获取当前节流间隔
JNIEXPORT jint JNICALL Java_com_tencent_yolov8ncnn_Yolov8Ncnn_getThrottleInterval(JNIEnv* env, jobject thiz)
{
    ncnn::MutexLockGuard g(lock);
    
    if (g_yolo)
    {
        return g_yolo->getThrottleInterval();
    }
    
    return 0;
}

// 设置检测模式
JNIEXPORT void JNICALL Java_com_tencent_yolov8ncnn_Yolov8Ncnn_setDetectMode(JNIEnv* env, jobject thiz, jint mode)
{
    ncnn::MutexLockGuard g(lock);
    
    if (g_yolo)
    {
        g_yolo->setDetectMode(mode);
        __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "setDetectMode %d", mode);
    }
}

// 获取当前检测模式
JNIEXPORT jint JNICALL Java_com_tencent_yolov8ncnn_Yolov8Ncnn_getDetectMode(JNIEnv* env, jobject thiz)
{
    ncnn::MutexLockGuard g(lock);
    
    if (g_yolo)
    {
        return g_yolo->getDetectMode();
    }
    
    return 0;
}

}
