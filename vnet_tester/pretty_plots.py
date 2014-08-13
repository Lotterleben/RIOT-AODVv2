import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid.axislines import Subplot
from operator import add

# TODO use as module

# some color constants (TODO: pick proper ones)
c_sucess_top = 'cadetblue'
c_sucess_bottom = 'Teal'
c_fail_top = 'LightCoral'
c_fail_bottom = 'DarkRed'
c_default = 'cadetblue'


def style_axes(ax):
    # remove borders
    ax.yaxis.grid()
    #ax.spines['left'].set_color('white')
    #ax.spines['top'].set_color('white')
    #ax.spines['right'].set_color('white')

    # remove ticks anywhere but at the bottom
    ax.xaxis.set_ticks_position('bottom')
    ax.yaxis.set_ticks_position('none')


'''
example input:
labels : ("exp1", "exp2")
fail =((fail bottom), (fail top))
success =(success)
'''
def plot_bars(legend, labels, success, fail):
    N = 2
    index_start = 0
    bar_padding = 0.2

    fail_bottom = fail[0]
    fail_top = fail[1]

    success_bottom = success[0]
    success_top = success[1]

    fig = plt.figure()
    ax = fig.gca()
    style_axes(ax)

    ind = np.arange(N)    # the x locations for the groups
    width = 0.35       # the width of the bars: can also be len(x) sequence
    fail_sums = map(add, fail_bottom, fail_top)
    success_sums = map(add, success_bottom, success_top)

    max_yval = (max(max(fail_sums), max(success_sums)))

    bar1_bottom = plt.bar(ind+width, success_bottom, width, color=c_sucess_bottom) #, edgecolor = "none")
    bar1_top = plt.bar(ind+width, success_top, width, bottom=success_bottom, color=c_sucess_top) #, edgecolor = "none")

    bar2_bottom = plt.bar(ind, fail_bottom, width, color=c_fail_bottom) #, edgecolor = "none")
    bar2_top = plt.bar(ind, fail_top, width, bottom=fail_bottom, color=c_fail_top) #, edgecolor = "none")
    #bar2_top = plt.bar(ind, fail_top, width, color=c_fail_top) #, edgecolor = "none

    #legend = ax.legend((bar1_bottom[0], bar1_top[0], bar2_bottom[0], bar2_top[0]), legend, loc=2)
    legend = ax.legend((bar1_top[0], bar2_top[0]), legend, loc=2)

    #legend = ax.legend((bar1_bottom[0], bar1_top[0], bar2_top[0]), legend, loc=2)
    legend.get_frame().set_edgecolor('white')

    plt.xticks(ind+width, labels)
    plt.ylabel("number of discoveries/transmissions")
    plt.xlim([index_start - bar_padding, N + bar_padding]) # don't make let bar stick to y axis
    plt.yticks(np.arange(0,max_yval,10))

    plt.savefig("plot_bars", format='pdf')
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

def plot_stacked_bars(legend, labels, group1, group2):
    N = 5
    menMeans   = (20, 35, 30, 35, 27)
    womenMeans = (25, 32, 34, 20, 25)

    ind = np.arange(N)    # the x locations for the groups
    width = 0.35       # the width of the bars: can also be len(x) sequence

    p0 = plt.bar(ind+width, group1,   width, color=c_sucess)
    p1 = plt.bar(ind, group2,   width, color=c_fail_1)
    p2 = plt.bar(ind, womenMeans, width, color=c_fail_2,
                 bottom=menMeans)

    max_yval = (max(max(group1), max(group2)))

    plt.ylabel('Scores')
    plt.title('Scores by group and gender')
    plt.xticks(ind+width/2., ('G1', 'G2', 'G3', 'G4', 'G5') )
    plt.yticks(np.arange(0,max_yval,10))
    plt.legend( (p1[0], p2[0]), ('Men', 'Women'))

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
    plot_stacked_bars(('failed', 'successful'), ("foo", "bar") ,(20, 35, 30, 35, 27), (25, 32, 34, 20, 25))
    #plot_curve([17,21,23,43, 5, 66], 'some numbers', 'some events')


