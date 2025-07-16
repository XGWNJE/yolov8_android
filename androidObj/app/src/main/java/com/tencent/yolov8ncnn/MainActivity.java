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

package com.tencent.yolov8ncnn;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ImageButton;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback
{
    public static final int REQUEST_CAMERA = 100;

    private Yolov8Ncnn yolov8ncnn = new Yolov8Ncnn();
    private int facing = 0;

    private Spinner spinnerModel;
    private Spinner spinnerCPUGPU;
    private Spinner spinnerDetectMode;
    private int current_model = 0;
    private int current_cpugpu = 0;
    private int current_detect_mode = 0;

    private SurfaceView cameraView;
    private TextView cameraErrorText;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        // requestWindowFeature(Window.FEATURE_NO_TITLE); // No longer needed with NoActionBar theme
        setContentView(R.layout.main);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        cameraView = (SurfaceView) findViewById(R.id.cameraview);
        cameraErrorText = (TextView) findViewById(R.id.camera_error_text);

        cameraView.getHolder().setFormat(PixelFormat.RGBA_8888);
        cameraView.getHolder().addCallback(this);

        ImageButton buttonSwitchCamera = (ImageButton) findViewById(R.id.buttonSwitchCamera);
        buttonSwitchCamera.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {

                int new_facing = 1 - facing;

                yolov8ncnn.closeCamera();

                boolean cameraOpened = yolov8ncnn.openCamera(new_facing);
                if (cameraOpened) {
                    facing = new_facing;
                    // 相机打开成功，隐藏错误文本
                    cameraErrorText.setVisibility(View.GONE);
                } else {
                    // 如果新的摄像头打开失败，尝试重新打开原来的摄像头
                    boolean reOpenSuccess = yolov8ncnn.openCamera(facing);
                    if (!reOpenSuccess) {
                        // 如果都失败了，提示用户
                        showCameraErrorDialog();
                    } else {
                        // 原相机重新打开成功，隐藏错误文本
                        cameraErrorText.setVisibility(View.GONE);
                    }
                }
            }
        });

        spinnerModel = (Spinner) findViewById(R.id.spinnerModel);
        spinnerModel.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> arg0, View arg1, int position, long id)
            {
                if (position != current_model)
                {
                    current_model = position;
                    reload();
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> arg0)
            {
            }
        });

        spinnerCPUGPU = (Spinner) findViewById(R.id.spinnerCPUGPU);
        spinnerCPUGPU.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> arg0, View arg1, int position, long id)
            {
                if (position != current_cpugpu)
                {
                    current_cpugpu = position;
                    reload();
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> arg0)
            {
            }
        });

        TextView textViewThreshold = (TextView) findViewById(R.id.text_view_threshold);
        SeekBar seekBarThreshold = (SeekBar) findViewById(R.id.seek_bar_threshold);
        seekBarThreshold.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                float threshold = progress / 100.f;
                textViewThreshold.setText(String.format("%.2f", threshold));
                yolov8ncnn.setConfidenceThreshold(threshold);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });
        
        // 设置节流控制的监听器
        TextView textViewThrottle = (TextView) findViewById(R.id.text_view_throttle);
        SeekBar seekBarThrottle = (SeekBar) findViewById(R.id.seek_bar_throttle);
        seekBarThrottle.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                // 显示当前节流间隔
                textViewThrottle.setText(String.format("%d ms", progress));
                // 设置模型推理的节流间隔
                yolov8ncnn.setThrottleInterval(progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });
        
        // 设置识别模式选择的监听器
        spinnerDetectMode = (Spinner) findViewById(R.id.spinner_detect_mode);
        spinnerDetectMode.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (position != current_detect_mode) {
                    current_detect_mode = position;
                    // 设置新的识别模式
                    yolov8ncnn.setDetectMode(position);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        reload();
    }

    private void reload()
    {
        boolean ret_init = yolov8ncnn.loadModel(getAssets(), current_model, current_cpugpu);
        if (!ret_init)
        {
            Log.e("MainActivity", "yolov8ncnn loadModel failed");
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
    {
        yolov8ncnn.setOutputWindow(holder.getSurface());
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder)
    {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
    }

    @Override
    public void onResume()
    {
        super.onResume();

        if (ContextCompat.checkSelfPermission(getApplicationContext(), Manifest.permission.CAMERA) == PackageManager.PERMISSION_DENIED)
        {
            ActivityCompat.requestPermissions(this, new String[] {Manifest.permission.CAMERA}, REQUEST_CAMERA);
        }

        boolean cameraOpened = yolov8ncnn.openCamera(facing);
        if (!cameraOpened) {
            // 相机打开失败，显示提示
            Log.e("MainActivity", "无法打开摄像头，可能是设备不支持NDK相机API");
            showCameraErrorDialog();
        } else {
            // 相机打开成功，隐藏错误文本
            cameraErrorText.setVisibility(View.GONE);
        }
    }
    
    /**
     * 显示相机错误对话框
     */
    private void showCameraErrorDialog() {
        // 显示错误文本
        cameraErrorText.setVisibility(View.VISIBLE);
        
        androidx.appcompat.app.AlertDialog.Builder builder = new androidx.appcompat.app.AlertDialog.Builder(this);
        builder.setTitle("相机错误");
        builder.setMessage("无法打开摄像头，可能是以下原因：\n\n1. 设备不支持NDK相机API\n2. 未授予相机权限\n3. 其他应用正在使用相机");
        builder.setPositiveButton("我知道了", null);
        builder.show();
    }
    
    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        
        if (requestCode == REQUEST_CAMERA) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                // 权限获取成功，尝试打开相机
                boolean cameraOpened = yolov8ncnn.openCamera(facing);
                if (!cameraOpened) {
                    // 相机打开失败，显示提示
                    showCameraErrorDialog();
                }
            } else {
                // 用户拒绝了权限
                androidx.appcompat.app.AlertDialog.Builder builder = new androidx.appcompat.app.AlertDialog.Builder(this);
                builder.setTitle("需要相机权限");
                builder.setMessage("此应用需要相机权限才能正常运行。请在设置中开启相机权限。");
                builder.setPositiveButton("我知道了", null);
                builder.show();
            }
        }
    }

    @Override
    public void onPause()
    {
        super.onPause();

        yolov8ncnn.closeCamera();
    }
}
