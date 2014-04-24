import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid.axislines import Subplot

# TODO use as module

# some color constants (TODO: pick proper ones)
c_sucess = 'cadetblue'
c_fail = 'LightCoral'
c_default = "cadetblue"


def style_axes(ax):
    # remove borders
    ax.yaxis.grid()
    ax.spines['left'].set_color('white')
    ax.spines['top'].set_color('white')
    ax.spines['right'].set_color('white')

    # remove ticks anywhere but at the bottom
    ax.xaxis.set_ticks_position('bottom')
    ax.yaxis.set_ticks_position('none')


'''
example input:
labels : ("exp1", "exp2")
group1 =(<values for 1st group>)
group2 =(<values for 2nd group>)
'''
def plot_bars(legend, labels, group1, group2):
    N = 2
    index_start = 0
    bar_padding = 0.2
    
    fig = plt.figure()
    ax = fig.gca()
    style_axes(ax)

    ind = np.arange(N)    # the x locations for the groups
    width = 0.35       # the width of the bars: can also be len(x) sequence
    max_yval = (max(max(group1), max(group2)))

    bar1 = plt.bar(ind+width, group1, width, color=c_sucess) #, edgecolor = "none")
    bar2 = plt.bar(ind, group2, width, color=c_fail) #, edgecolor = "none")

    legend = ax.legend((bar1[0], bar2[0]), legend)
    legend.get_frame().set_edgecolor('white') # TODO amke borders of colors go away

    plt.xticks(ind+width, labels)
    plt.xlim([index_start - bar_padding, N + bar_padding]) # don't make let bar stick to y axis
    plt.yticks(np.arange(0,max_yval,10))

    plt.show()

'''
input:
values : list
ylabel, xlabel: string
'''
def plot_curve(values, ylabel, xlabel):

    fig = plt.figure()
    ax = fig.gca()
    style_axes(ax)

    plt.plot(values, color=c_default)
    plt.ylabel(ylabel)
    plt.xlabel(xlabel)

    plt.show()
    

def plot_test():

    # style
    fig = plt.figure()
    ax = Subplot(fig, 111)
    fig.add_subplot(ax)

    ax.axis["right"].set_visible(False)
    ax.axis["top"].set_visible(False)

    # values
    plt.plot([1,2,3,4])
    
    # context
    plt.ylabel('some numbers')

    plt.show()


if __name__ == "__main__":
    #plot_test()
    #plot_bar(('failed', 'successful'), (20, 35, 30, 35, 27), (25, 32, 34, 20, 25))
    plot_curve([17,21,23,43, 5, 66], 'some numbers', 'some events')


