# NOTE:
#   omni.kit.test - std python's unittest module with additional wrapping to add suport for async/await tests
#   For most things refer to unittest docs: https://docs.python.org/3/library/unittest.html
import omni.kit.test

# Import extension python module we are testing with absolute import path, as if we are external user (other extension)
from add_on.xr.openxr import _openxr

import cv2
import time
import numpy as np

# Having a test class dervived from omni.kit.test.AsyncTestCase declared on the root of module will make it auto-discoverable by omni.kit.test
class TestOpenXR(omni.kit.test.AsyncTestCaseFailOnLogError):
    # Before running each test
    async def setUp(self):
        pass

    # After running each test
    async def tearDown(self):
        pass

    def render(self, num_frames, width, height, color):
            self._frame = np.ones((height, width, 3), dtype=np.uint8)
            self._frame = cv2.circle(self._frame, (int(width / 2), int(height / 2)), 
                                     int(np.min([width, height]) / 4), color, int(0.05 * np.min([width, height])))
            self._frame = cv2.circle(self._frame, (int(width / 3), int(height / 3)), 
                                     int(0.1 * np.min([width, height])), color, -1)
            
            start_time = time.clock()

            for i in range(num_frames):
                if self._xr.poll_events():
                    if self._xr.is_session_running():
                        if not self._xr.poll_actions():
                            print("[ERROR]:", "pollActions")
                            return
                        if not self._xr.render_views():
                            print("[ERROR]:", "renderViews")
                            return

            end_time = time.clock()
            delta = end_time - start_time

            print("----------")
            print("FPS: {} ({} frames / {} seconds)".format(num_frames / delta, num_frames, delta))
            print("RESOLUTION: {} x {}".format(self._frame.shape[1], self._frame.shape[0]))

    # Actual test, notice it is "async" function, so "await" can be used if needed
    async def test_openxr(self):
        self._xr = _openxr.acquire_openxr_interface()
        self._xr.init()

        ready = False

        if self._xr.create_instance():
            if self._xr.get_system():
                if self._xr.create_action_set():
                    if self._xr.create_session():
                        ready = True
                    else:
                        print("[ERROR]:", "createSession")
                else:
                    print("[ERROR]:", "createActionSet")
            else:
                print("[ERROR]:", "getSystem")
        else:
            print("[ERROR]:", "createInstance")

        if ready:
            for i in range(1000):
                if self._xr.poll_events():
                    if self._xr.is_session_running():
                        if not self._xr.poll_actions():
                            print("[ERROR]:", "pollActions")
                            break


        # if ready:
        #     def callback_render(num_views, views, configuration_views):
        #         self._xr.set_frames(configuration_views, self._frame, self._frame, self._transform)

        #     self._xr.subscribe_render_event(callback_render)

        #     num_frames = 100

        #     print("")
        #     print("transform = True")
        #     self._transform = True
        #     self.render(num_frames=num_frames, width=1560, height=1732, color=(255,0,0))
        #     self.render(num_frames=num_frames, width=1280, height=720, color=(0,255,0))
        #     self.render(num_frames=num_frames, width=500, height=500, color=(0,0,255))
        #     print("")
        #     print("transform = False")
        #     self._transform = False
        #     self.render(num_frames=num_frames, width=1560, height=1732, color=(255,0,0))
        #     self.render(num_frames=num_frames, width=1280, height=720, color=(0,255,0))
        #     self.render(num_frames=num_frames, width=500, height=500, color=(0,0,255))
        #     print("")

        #     _openxr.release_openxr_interface(self._xr)
        #     self._xr = None
