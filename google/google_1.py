def answer(x, y):
    if len(x) != len(y):
        raise ValueError('Experiment data not equal')
    
    xsum = sum(x)
    ysum = sum(y)
    if ysum == 0:
        raise ValueError('Something is bad with y')
    ret_raw = (1 - (xsum / ysum)) * 100
    return int(round(abs(ret_raw)))
