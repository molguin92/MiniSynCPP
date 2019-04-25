import pandas
from matplotlib import pyplot as plt

if __name__ == '__main__':
    df = pandas.read_csv("./stats.csv", sep=';', index_col=False)
    # df = df.drop([0])
    # df.plot(x='Sample', y='Drift', yerr='Drift Error')
    fig, ax = plt.subplots()
    ax.plot(df['Sample'], df['Drift'], label='Drift', color='blue')
    ax.fill_between(df['Sample'],
                    df['Drift'] - df['Drift Error'],
                    df['Drift'] + df['Drift Error'],
                    color='gray', alpha=0.2)
    ax.legend()
    # ax.set_yscale('symlog', basey=10)
    ax.set_xscale('symlog', basex=10)
    plt.show()
    fig.savefig('drift.png')

    fig, ax = plt.subplots()
    offsets_ms = (df['Offset']) / 1000.0
    e_offsets_ms = df['Offset Error'] / 1000.0

    ax.plot(df['Sample'], offsets_ms, label='Offset [ms]', color='red')
    ax.fill_between(df['Sample'],
                    offsets_ms - e_offsets_ms,
                    offsets_ms + e_offsets_ms,
                    color='gray', alpha=0.2)
    # ax.set_yscale('symlog', basey=10)
    ax.set_xscale('symlog', basex=10)
    ax.legend()
    plt.show()
    fig.savefig('offset.png')
