## 项目介绍

本项目是基于NCNN框架的YOLOv8目标检测与分割演示，包含：

1. Android YOLOv8检测演示应用
2. YOLOv8s分割模型实现
3. YOLOv8s有向边界框(OBB)检测实现

## 我的修改内容

在原项目基础上，我进行了以下修改和优化：

- 优化了UI布局，提升用户体验
- 新增置信度控制功能，允许用户调整目标检测的置信度阈值
- 添加节流控制功能，优化性能和资源占用
- 实现识别对象分组功能，使检测结果更加条理化

## 项目结构

- `ncnn-android-yolov8`: Android应用演示
- `ncnn-yolov8s-seg`: YOLOv8分割模型实现
- `ncnn-yolov8s-obb`: YOLOv8有向边界框检测实现



## 如何使用

### 环境要求
- Android Studio
- NDK
- NCNN库
- OpenCV移动版

### 编译与运行
1. 克隆本仓库
2. 按照ncnn-android-yolov8目录中的README说明配置环境
3. 使用Android Studio打开项目并运行

## 参考资料
- [ncnn-android-nanodet](https://github.com/nihui/ncnn-android-nanodet)
- [Tencent NCNN](https://github.com/Tencent/ncnn)
- [Ultralytics YOLOv8](https://github.com/ultralytics/assets/releases/tag/v0.0.0)
- [Detectron2](https://github.com/facebookresearch/detectron2)

## License

本项目遵循原项目的License。

## 原项目声明

原项目地址：[https://github.com/FeiGeChuanShu/ncnn-android-yolov8](https://github.com/FeiGeChuanShu/ncnn-android-yolov8)

原作者：[@FeiGeChuanShu](https://github.com/FeiGeChuanShu)