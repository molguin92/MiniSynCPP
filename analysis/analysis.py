import pandas
from matplotlib import pyplot as plt

if __name__ == '__main__':
    df = pandas.read_csv("./stats.csv", sep=';', index_col=False)
    df = df.drop([0])
    df.plot(x='Sample', y='Drift', yerr='Drift Error')
    fig, ax = plt.subplots()
    df.plot(x='Sample', y='Offset', yerr='Offset Error', ax=ax)
    ax.set_yscale('log')

    # plot time vs adjusted time
    # adj_time = ((df['Drift'].iloc[-1] * df['Timestamp'])
    #             + df['Offset'].iloc[-1]) / 1000000.0
    # timestamps = df['Timestamp'] / 1000000.0
    # fig, ax = plt.subplots()
    # ax.plot(timestamps, adj_time)
    # ax.plot(timestamps, timestamps)
    # ax.ticklabel_format(useOffset=False)
    plt.show()
