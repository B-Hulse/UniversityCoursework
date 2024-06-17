ffmpeg -y -r 60 -f image2 -s 750x750 -i outpng%%d.png -vcodec libx264 -crf 15  -pix_fmt yuv420p outVid_flip.mp4
ffmpeg -y -i outVid_flip.mp4 -vf vflip -c:a copy outVid.mp4
del outVid_flip.mp4
PAUSE