# coding: utf-8

#v0.1.0  修改程序异常不自动退出，等待一段时间循环尝试   20180301
#v0.1.1  count字符错误 20180529

import time
import pymongo
import logging
import datetime
import bisect


def manyapname(early_time_str):
    aset = set()   
    for item in unique_mac.find({'end_time': {'$gte': early_time_str}}, {'_id': 0, 'apname_no_empty': 1}):
        aset.add(item['apname_no_empty'])
    return aset


def get_manu_flag(manyap_items):
    manu_flag = None
    if manyap_items.count() < 2:
        item = next(manyap_items)
        if item.get('flag', None):
            return None
        if 'UnKnow' in item['manu']:
            manu_flag = '1IOS+' + str(hash(item['mac']))
        elif 'Apple' in item['manu']:
            manu_flag = 'IOS+' + str(hash(item['mac']))
        return manu_flag

    for i in manyap_items:
        if 'UnKnow' in i['manu']:
            continue
        elif 'Apple' in i['manu']:
            manu_flag = 'IOS+' + str(hash(i['mac']))
        else:
            return None

    if not manu_flag:
        manu_flag = 'IOS+' + str(hash(i['mac']))

    return manu_flag


def getmidtime(start, end):
    startstamp = int(time.mktime(time.strptime(start, "%Y-%m-%d %H:%M:%S")))
    endstamp = int(time.mktime(time.strptime(end, "%Y-%m-%d %H:%M:%S")))

    midtime = time.strftime("%Y-%m-%d %H:%M:%S",
                            time.localtime((startstamp + endstamp) // 2))
    return midtime


def update_item(item_info):
    update_apname_flag = False
    mac = item_info['mac']
    curren_time_str = item_info['reqtime']
    apname = item_info['apname']
    station = item_info['station']
    _id = item_info['_id']
    macList.update_one({'_id': _id}, {"$set": {'newitem': False}})

    curren_time_stamp = int(time.mktime(time.strptime(curren_time_str, "%Y-%m-%d %H:%M:%S")))
    ten_min_before_time = time.strftime("%Y-%m-%d %H:%M:%S",
                                        time.localtime(curren_time_stamp - 10 * 60))  # before 10min

    ten_min_after_time = time.strftime("%Y-%m-%d %H:%M:%S",
                                       time.localtime(curren_time_stamp + 10 * 60))  # after 10min

    is_middle_time_item = unique_mac.find_one({         # find before 10min item,if have ,statrtime is item's start_time
        '$and': [{
            'end_time': {'$gte': curren_time_str},
            'start_time': {'$lte': curren_time_str},
            'mac': mac,
            'station': station}]
    })


    if is_middle_time_item:
        id = is_middle_time_item['_id']
        counts = is_middle_time_item['counts'] + 1
        apname_list = is_middle_time_item['apname_list']
        if apname not in apname_list:
            bisect.insort(apname_list, apname)
            apname_no_empty = ','.join((i for i in apname_list if i != ''))
            update_apname_flag = True

        if update_apname_flag:
            unique_mac.update_one({'_id': id}, {'$set': {'apname_list': apname_list, 'apname_no_empty': apname_no_empty, 'counts': counts}})
        else:
            unique_mac.update_one({'_id': id}, {'$set': {'counts': counts}})
        return

    else:
        has_start_time_item = unique_mac.find_one({  # find before 10min item,if have ,statrtime is item's start_time
            '$and': [{
                'end_time': {'$gte': ten_min_before_time, '$lte': curren_time_str},
                'mac': mac,
                'station': station}]
        })

        has_end_time_item = unique_mac.find_one({
            '$and': [{
                'start_time': {'$lte': ten_min_after_time, '$gt': curren_time_str},
                'mac': mac,
                'station': station}]
        })

        if has_start_time_item and has_end_time_item:
            id = [has_start_time_item['_id'], has_end_time_item['_id']]
            start_time = has_start_time_item['start_time']
            end_time = has_end_time_item['end_time']
            counts = has_start_time_item['counts'] + has_end_time_item['counts'] + 1
            apname_set = set(has_start_time_item['apname_list']) | set(has_end_time_item['apname_list'])
            unique_mac.delete_many({'_id': {'$in': id}})

        elif has_start_time_item:
            id = has_start_time_item['_id']
            start_time = has_start_time_item['start_time']
            end_time = curren_time_str
            mid_time = getmidtime(start_time, end_time)
            counts = has_start_time_item['counts'] + 1
            apname_list = has_start_time_item['apname_list']
            if apname not in apname_list:
                bisect.insort(apname_list, apname)
                apname_no_empty = ','.join((i for i in apname_list if i != ''))
                update_apname_flag = True

            if update_apname_flag:
                unique_mac.update_one({'_id': id}, {
                    '$set': {"counts": counts, "end_time": end_time, "midtime": mid_time, "apname_list": apname_list,
                             "apname_no_empty": apname_no_empty}})
            else:
                unique_mac.update_one({'_id': id}, {'$set': {"counts": counts, "end_time": end_time, "midtime": mid_time}})

            return

        elif has_end_time_item:
            id = has_end_time_item['_id']
            start_time = has_end_time_item['start_time']
            end_time = curren_time_str
            mid_time = getmidtime(start_time, end_time)
            counts = has_end_time_item['counts'] + 1
            apname_list = has_end_time_item['apname_list']
            if apname not in apname_list:
                bisect.insort(apname_list, apname)
                apname_no_empty = ','.join((i for i in apname_list if i != ''))
                update_apname_flag = True

            if update_apname_flag:
                unique_mac.update_one({'_id': id}, {
                    '$set': {"counts": counts, "start_time": start_time, "midtime": mid_time, "apname_list": apname_list,
                             "apname_no_empty": apname_no_empty}})
            else:
                unique_mac.update_one({'_id': id}, {'$set': {"counts": counts, "start_time": start_time, "midtime": mid_time}})

            return

        else:
            start_time = curren_time_str
            end_time = curren_time_str
            counts = 1
            apname_set = {apname}

        mid_time = getmidtime(start_time, end_time)
        apname_list = sorted(list(apname_set))
        apname_no_empty = ','.join((i for i in apname_list if i != ''))

        unique_mac.insert_one(
            {'mac': mac, 'apname_list': apname_list, 'apname_no_empty': apname_no_empty,
             'counts': counts, "flag": None,
             'manu': item_info['manu'], 'station': station, 'start_time': start_time, 'end_time': end_time, 'mid_time': mid_time})

        return


def log(filename):
    mylogger = logging.getLogger('macname')
    logformat = '%(levelname)s %(name)-12s %(asctime)s: %(message)s'
    logging.basicConfig(filename=filename, format=logformat)

    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.ERROR)
    mylogger.addHandler(console_handler)

    return mylogger


def main():
    while True:
        for index in macList.find({'newitem': True}):
            update_item(index)

        interval = 24 * 2  # two day early
        early_time = datetime.datetime.now() - datetime.timedelta(hours=interval)
        early_time_str = early_time.strftime("%Y-%m-%d %H:%M:%S")

        unque_manyap_set = manyapname(early_time_str)

        for index in unque_manyap_set:
            # manyap_items = unique_mac.find({'end_time': {"$gte": early_time_str}, 'apname_no_empty': index}, {"manu": 1, "flag": 1, "_id": 1})
            manyap_items = unique_mac.find({'apname_no_empty': index}, {"manu": 1, "flag": 1, "_id": 1, "mac": 1})
            flag = get_manu_flag(manyap_items)
            if flag:
                unique_mac.update_many({'apname_no_empty': index, 'end_time': {"$gte": early_time_str}}, {"$set": {'flag': flag}})
                # unique_mac.update_many({'apname_no_empty': index}, {"$set": {'flag': flag}})
        print('--------------')
        time.sleep(5)


if __name__ == "__main__":
    client = pymongo.MongoClient('127.0.0.1', 27017)
    router = client['router']
    macList = router['init_data']
    unique_mac = router['unique_mac']
    logger = log('macname.log')

    while True:
        try:
            main()
        except pymongo.errors.ConnectionFailure as e:
            logger.error("mongodb connect timeout")
            time.sleep(5)
        except Exception as e:
            logger.exception(e)
            # no exit,wait 60s to retry
            time.sleep(60)


