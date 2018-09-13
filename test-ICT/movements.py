import Tkinter as tk
import itertools
import math
import random
import os
import sys
import errno
from time import sleep
from Tkinter import *

class Socket:
    def __init__(self, path = "/tmp/test.fifo", buffer_size = 100):
        self.buffer_size = buffer_size
        try:
            os.mkfifo(path, 0777)
        except OSError as err:
            #it's okay only if fifo is already open
            if err.errno != 17:
                raise err;
        self.pipe = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
        #self.fifo=open(path,'rb')

    def read(self):
        return(os.read(self.pipe, self.buffer_size))
        #return(self.fifo.read())

class Constants:
    SLICES = 50           # how many slices we should split a single grid box into
    REFRESH_RATE = 15     # in milliseconds, best effort by tkinter scheduler

class NodeType:
    STATIONARY = 0
    MOBILE = 1

class Node:
    def __init__(self, canvas, name, row, column, comm_radius, grid_size,
                 velocity = 0, direction = None, targets = [],
                 node_type = NodeType.STATIONARY, node_width = 0.5,
                 node_color = "gray"):
        self.canvas = canvas
        self.name = name
        self.center = (column * grid_size, row * grid_size)
        self.comm_radius = comm_radius * grid_size
        self.velocity = velocity
        self.direction = direction
        self.targets = targets
        self.node_type = node_type
        self.node_width = node_width * grid_size
        self.node_color = node_color
        self.canvas_items = [ ]

        if self.node_type == NodeType.STATIONARY:
            self.step_size = None
        else:
            # unit step size is 1/50 of the grid size in one go
            # however, this nodes step size will be affected by their speed
            # also note that this will be visually affected by whatever refresh
            # rate is chosen, so we tend on the side of fine-grained control
            self.step_size = velocity * (grid_size / Constants.SLICES)

        if len(targets) > 0:
            self.target_idx = 0
        else:
            self.target_idx = None

        # draw the initial shapes -- this is the only time stationary nodes are drawn
        self.draw()

    def draw(self):
        # delete old nodes in canvas items, and reset canvas_items list for new entries
        for item in self.canvas_items:
            self.canvas.delete(item)
        self.canvas_items = [ ]

        # draw center circle
        x_center, y_center = self.center
        x0 = x_center - self.comm_radius
        y0 = y_center - self.comm_radius
        x1 = x_center + self.comm_radius
        y1 = y_center + self.comm_radius
        delta = self.node_width / 2.0
        item = self.canvas.create_oval(x_center - delta, y_center - delta,
                                     x_center + delta, y_center + delta,
                                     fill = self.node_color)
        self.canvas_items.append(item)
        # draw communication radius
        # item = self.canvas.create_oval(x0, y0, x1, y1, outline = self.node_color)
        # self.canvas_items.append(item)

        if self.node_type == NodeType.MOBILE:
            if self.velocity <= 0:
                return
            # draw direction vector
            rads = math.radians(self.direction)
            length = 10 * self.velocity
            x_tip = x_center + length * math.cos(rads)
            y_tip = y_center - length * math.sin(rads)
            item = self.canvas.create_line(x_center, y_center, x_tip, y_tip, arrow="last")
            self.canvas_items.append(item)

    def erase(self):
        for item in self.canvas_items:
            self.canvas.delete(item)

    # Does our center point lie within the communication circle of another node?
    # This function is optimized for points being OUTSIDE of the circle.
    def in_range(self, node):
        if self == node:
            return None
        x0, y0 = self.center # my center point
        x1, y1 = node.center # other node's center point
        # calculate distances
        dx = abs(x0 - x1)
        dy = abs(y0 - y1)

        # if it is outside the square drawn around the circle, it is outside
        if (dx > node.comm_radius) or (dy > node.comm_radius):
            return False
        # if it is inside the diamond inscribed in the circle, it is inside
        if (dx + dy <= node.comm_radius):
            return True
        # now, worst case, we need to compute the circle
        if ( (dx**2) + (dy**2) ) <= (node.comm_radius**2):
            return True
        return False

    def get_target(self):
        return self.targets[ self.target_idx ]

    def move_next_target(self):
        self.target_idx += 1
        if self.target_idx >= len(self.targets):
            self.target_idx = 0
        return self.targets[ self.target_idx ]

class Simulation(tk.Frame):
    def __init__(self, root, rows=16, columns=16, grid_size=30,
                 num_stationary=1, num_mobile=4, seed=0):
        self.socket = Socket()
        self.oldRead = None
        self.num_rows = rows
        self.num_cols = columns
        self.grid_size = grid_size # in pixels

        self.num_stationary = num_stationary
        self.num_mobile = num_mobile
        self.nodes = [ ]

        random.seed(seed)

        canvas_width = columns * grid_size
        canvas_height = rows * grid_size

        tk.Frame.__init__(self, root)
        self.canvas = tk.Canvas(self, borderwidth=0, highlightthickness=0,
                                width=canvas_width, height=canvas_height)
        self.canvas.pack(side="top", fill="both", expand=True) ## , padx=10, pady=10)

        # draw grid lines
        for row in range(self.num_rows):
            for col in range(self.num_cols):
                x1 = (col * self.grid_size)
                y1 = (row * self.grid_size)
                x2 = x1 + self.grid_size
                y2 = y1 + self.grid_size
                self.canvas.create_rectangle(x1, y1, x2, y2, outline="light blue", tags="square")

        self.add_nodes()
        self.visual_loop()

    def add_nodes(self):
        # add stationary nodes
        for i in range(self.num_stationary):
            self.nodes.append( Node(self.canvas,
                                    "s" + str(i),
                                    # random.random() * self.num_rows,
                                    # random.random() * self.num_cols,
                                    (self.num_rows)/2,
                                    (self.num_cols)/2,
                                    random.random() * 4 + 2,
                                    self.grid_size,
                                    node_color = "royal blue") )
        print (self.num_rows)/2
        print (self.num_cols)/2
        stationary_targets = [ node.center for node in self.nodes ]
        mobile_nodes_ids = [ "h9x1" , "h2x1", "h8x1", "h12x2" ]
        # add mobile nodes
        for i in range(self.num_mobile):
            self.nodes.append( Node(self.canvas,
                                    mobile_nodes_ids[i], #+ str(i),
                                    3,
                                    5,
                                    # random.random() * self.num_rows,
                                    # random.random() * self.num_cols,
                                    random.random() * 3 + 0.5,
                                    self.grid_size,
                                    #velocity = random.random() * 3 + 0.5,
                                    #direction = random.random() * 360,
                                    #targets = stationary_targets,
                                    node_type = NodeType.MOBILE) )

    def visual_loop(self):
        self.move_all_nodes()
        #self.check_ranges()
        # stop updating graphics if only stationary nodes are left
        if len(self.nodes) > self.num_stationary:
            self.after(Constants.REFRESH_RATE, self.visual_loop)

    def move_all_nodes(self):
        # copy the node list so we can delete from self.nodes as we iterate

        try:
            str=self.socket.read()
        except OSError as err:
            if err.errno == errno.EAGAIN or err.errno == errno.EWOULDBLOCK:
                str = None
            else:
                raise  # something else has happened -- better reraise

        if str is not None:
            # buffer contains some received data -- do something with it
            #print "Read:",str
            if str != self.oldRead and str != "":

                # first, split num of inputs
                inputLine = str.split("|")
                for line in inputLine:
                    #print "Read:",str
                    input = line.split(":")
                    if len(input) != 3:
                        continue
                    print("Input: Node {0}: center ({1}, {2})".format(input[0], input[1], input[2]))

                    in_name = input[0]
                    in_x = int(input[1])
                    in_y = int(input[2])

                    node_list = self.nodes[:]
                    for node in node_list:
                        if node.node_type == NodeType.STATIONARY:
                            continue
                        if node.name == in_name:
                            print("Node {0}: center ({1}, {2})".format(node.name, node.center[0], node.center[1]))
                            target = in_x, in_y
                            dist, rads = self.point_to_point_distance_angle(node.center, target)
                            node.direction = rads
                            move_x, move_y = target
                            node.center = (move_x, move_y)
                            node.draw()

                            #delete node if it's outside the grid
                            # if self.check_node_outside_grid(node):
                            #     node.erase()
                            #     self.nodes.remove(node)
                            #     del node

                    # x_center, y_center = node.center
                    # target = x_center+1, y_center+2
                    # dist, rads = self.point_to_point_distance_angle(node.center, target)
                    # node.direction = rads
                    # move_x, move_y = target
                    # node.center = (move_x, move_y)
                    # node.draw()

        self.oldRead = str
        #fifo.close()



            # # if a node has targets, use those to set direction
            # if node.targets:
            #     print("Node {0}: center ({1}, {2})".format(node.name, node.center[0], node.center[1]))
            #     target = node.get_target()
            #     print("Node {0}: target ({1}, {2})".format(node.name, target[0], target[1]))
            #     dist, rads = self.point_to_point_distance_angle(node.center, target)
            #     print("Node {0}: distance {1}, direction {2})".format(node.name, dist, rads) )
            #     node.direction = rads
            #     if dist < node.step_size:
            #         move_x, move_y = target
            #     else:
            #         x_center, y_center = node.center
            #         move_x = x_center + node.step_size * math.cos(rads)
            #         move_y = y_center + node.step_size * math.sin(rads)
            #         print("Node {0}: new coord ({1}, {2})".format(node.name, move_x, move_y))
            # else:
            #     # randomly stop & start nodes
            #     if random.random() < 0.1:
            #         # if node is moving, stop it
            #         if node.step_size > 0:
            #             node.step_size = 0
            #         else:
            #             node.step_size = node.velocity * (self.grid_size / Constants.SLICES)
            #
            #     # randomly change direction and velocity
            #     if random.random() < 0.01:
            #         node.velocity = random.random() * 3 + 0.5
            #         node.direction = random.random() * 360
            #
            #     # need to calculate one step size in the correct direction
            #     x_center, y_center = node.center
            #     rads = math.radians(node.direction)
            #     move_x = x_center + node.step_size * math.cos(rads)
            #     move_y = y_center - node.step_size * math.sin(rads)
            #
            # # move the node's center to here and redraw
            # node.center = (move_x, move_y)
            # node.draw()
            #
            # delete node if it's outside the grid
            # if self.check_node_outside_grid(node):
            #     node.erase()
            #     self.nodes.remove(node)
            #     del node

    def point_to_point_distance_angle(self, p1, p2):
        x1, y1 = p1
        x2, y2 = p2
        dist = math.sqrt( (x2 - x1)**2 + (y2 - y1)**2 )
        angle = math.atan2( (y2 - y1), (x2 - x1) )
        return dist, angle

    # this function is for a FINITE line segment, not a line defined by two points
    def point_distance_to_line_segment(self, p1, p2, node_center):
        x1, y1 = p1
        x2, y2 = p2
        x_center, y_center = node_center

        segment_len_squared = (x2 - x1)**2 + (y2 - y1)**2
        # avoids dividing by zero during projection
        if segment_len_squared == 0:
            return math.sqrt( (x2 - x_center)**2 + (y2 - y_center)**2 )

        # project node onto line created by line segment
        scale = ( (x_center-x1)*(x2-x1) + (y_center-y1)*(y2-y1) ) / segment_len_squared
        clamped_scale = max( 0, min(1, scale) )
        x_proj = x1 + clamped_scale * (x2 - x1)
        y_proj = y1 + clamped_scale * (y2 - y1)

        # calculate distance to projected point
        return math.sqrt( (x_proj - x_center)**2 + (y_proj - y_center)**2 )


    # this check is for a node
    def check_node_outside_grid(self, node):
        x_center, y_center = node.center
        # the four corners of our grid rectangle
        x1 = 0
        y1 = 0
        x2 = self.num_cols * self.grid_size
        y2 = 0
        x3 = 0
        y3 = self.num_rows * self.grid_size
        x4 = self.num_cols * self.grid_size
        y4 = self.num_rows * self.grid_size

        # first, check if center is outside or inside of grid rectangle
        if x_center >= x1 and x_center <= x4 and \
           y_center >= y1 and y_center <= y4:
            return False

        # its outside, but the communication radius may still overlap
        # use formula for shortest distance between point and a line,
        # but we don't know a priori which side of the rectangle the
        # node is close to, so we calculate them all and take the minimum
        d1 = self.point_distance_to_line_segment( (x1, y1), (x2, y2), node.center )
        d2 = self.point_distance_to_line_segment( (x1, y1), (x3, y3), node.center )
        d3 = self.point_distance_to_line_segment( (x4, y4), (x2, y2), node.center )
        d4 = self.point_distance_to_line_segment( (x4, y4), (x3, y3), node.center )
        dist = min(d1, d2, d3, d4)
        return dist >= node.comm_radius

    def check_range_pair(self, node0, node1):
        if not node0.in_range(node1):
            return False
        if not node1.in_range(node0):
            return False
        return True

    def check_ranges(self):
        for node0, node1 in itertools.combinations(self.nodes, 2):
            if self.check_range_pair(node0, node1):
                self.do_something(node0, node1)

    def do_something(self, node0, node1):
        print "Node {0} and Node {1} are in range.".format(node0.name, node1.name)
        # if a mobile node has converged to a stationary target, switch to the next one
        if node0.targets and not node1.targets:
            if node1.center == node0.targets[ node0.target_idx ]:
                node0.move_next_target()
        elif node1.targets and not node0.targets:
            if node0.center == node1.targets[ node1.target_idx ]:
                node1.move_next_target()


if __name__ == "__main__":
    root = tk.Tk()
    # f1 = Frame(root, height=100, width=100)
    # f1.pack()
    board = Simulation(root, grid_size=50, seed=10)
    board.pack(side="top", fill="both", expand="true")
    root.mainloop()
