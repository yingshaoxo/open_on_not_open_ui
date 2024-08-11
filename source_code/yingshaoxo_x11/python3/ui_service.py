from auto_everything.http_ import Yingshaoxo_Http_Server, Yingshaoxo_Http_Request, Yingshaoxo_Threading_Based_Http_Server

from auto_everything.image import Image
a_image = Image()

import time
from multiprocessing import Manager

#@dataclass()
#class Yingshaoxo_Http_Request():
#    context: Any
#    host: str
#    method: str
#    url: str
#    url_arguments: dict[str, str]
#    headers: dict[str, str]
#    payload: Any | None

manager = Manager()
window_size_list = manager.list([320, 240])
# the image resize or pixel iteration in python is 60 times slower than c version, so don't use it as much as possible
display_image_string_list = manager.list([None])

image_list = []
for name in ["water.png", "hero.png"]:
    image_list.append(a_image.read_image_from_file("/home/yingshaoxo/Downloads/" + name).copy().resize(window_size_list[0], window_size_list[1]))

def home_handler(request: Yingshaoxo_Http_Request):
    return "Hello, world, fight for personal freedom."

def download_picture(request: Yingshaoxo_Http_Request):
    global a_image, window_size_list, display_image_string_list
    # if you return a dict, will return a json, but here we do not need to return a json, a pure string is fine
    if display_image_string_list[0] == None:
        index = int(int(time.time()) % 2)
        height, width = window_size_list
        print(height, width)
        image_string = image_list[index].resize(height, width).save_image_as_string(bgra=True)
        return image_string
    else:
        return display_image_string_list[0]

def handle_input(request: Yingshaoxo_Http_Request):
    global window_size_list
    data_string = request.payload.decode("utf-8")
    lines = data_string.split("\n")
    a_dict = {}
    for line in lines:
        key,value = line.split("=")
        a_dict[key] = value
    window_size_list[0] = int(a_dict["height"])
    window_size_list[1] = int(a_dict["width"])
    return ""

def upload_picture(request: Yingshaoxo_Http_Request):
    global display_image_string_list
    a_data = request.payload
    #print(type(a_data))
    #print("got picture length: ", len(a_data))
    display_image_string_list[0] = a_data
    return ""

router = [
    [r"/download_picture", download_picture],
    [r"/handle_input", handle_input],
    [r"/upload_picture", upload_picture],
    [r"(.*)", home_handler]
]

yingshaoxo_http_server = Yingshaoxo_Http_Server(router=router)
#yingshaoxo_http_server = Yingshaoxo_Threading_Based_Http_Server(router=router)
yingshaoxo_http_server.start(host="localhost", port=7777)
