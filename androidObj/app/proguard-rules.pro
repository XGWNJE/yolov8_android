# 保留 native 方法
-keepclasseswithmembernames class * {
    native <methods>;
}

# 保留 R 类
-keep class **.R$* {
    *;
}

# 保留主 Activity
-keep public class com.tencent.yolov8ncnn.MainActivity

# 保留 Yolov8Ncnn 类
-keep public class com.tencent.yolov8ncnn.Yolov8Ncnn {
    *;
}

# 保留 ncnn 相关类
-keep class org.opencv.** { *; }
-keep class com.tencent.** { *; } 