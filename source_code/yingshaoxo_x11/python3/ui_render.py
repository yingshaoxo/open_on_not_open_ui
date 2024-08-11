from auto_everything.http_ import Yingshaoxo_Http_Client
yingshaoxo_http_client = Yingshaoxo_Http_Client()

from auto_everything.image import Image
a_image = Image()

import time

target_url = "http://localhost:7777/"

picture = a_image.read_image_from_file("/home/yingshaoxo/Downloads/" + "water.png").copy().resize(320, 240).save_image_as_string(bgra=True)
print("picture length: ", len(picture))

while True:
    try:
        yingshaoxo_http_client.post(target_url+"upload_picture", picture)
    except Exception as e:
        print(e)
    time.sleep(1)
