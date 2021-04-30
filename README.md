# Pixzo

## Abstract

There is a growing need to capture and process high resolution images and videos and to extract as much information as possible from them. There are many different cameras and software that can accompish this task, but there is a motivation to achive the highest performace level. A high resolution USB camera has been used to capture 4K video which has been handled by a computer in realtime to perform operations on each individual frame. Although a greate amount of information was transmitted, we were able to accurately process each frame as fats as possible with a custom driver implementation.

## Objective

Create a custom software that can retreive information from a USB video camera with a high frame rate when working with high resolution cameras and there is the need to perform intensive operations using each frame and to keep them in a highly optimized memory structure for a custom amount of time. The operations should be performed in parallel and the frames ultimately will be saved for long term storage. This solution should be reliable and efficient as it might be implemented with more than one camera in the real world in systems that might not be as powerful.

## References

- **OpenCV** - Bradski, G., 2000. The OpenCV Library. Dr. Dobb&#x27;s Journal of Software Tools.
- **FFmpeg** - FFmpeg Developers. (2016). ffmpeg tool (Version be1d324) \[Software\]. Available from http://ffmpeg.org
