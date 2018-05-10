# coding: utf-8

import time

import pymongo
import logging
import datetime
import bisect

from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from modules import Unimac, Init_sta1, init_db
from sqlalchemy import or_, and_
from sqlalchemy import text
from sqlalchemy import distinct

import pymysql
pymysql.install_as_MySQLdb()

DB_CONNECT_STRING = "mysql+mysqldb://root:123@192.168.3.108:3306/router"
engine = create_engine(DB_CONNECT_STRING, encoding="utf-8", echo=True)
DB_Session = sessionmaker(bind=engine)
session = DB_Session()

init_db(engine)

import datetime
now = datetime.datetime.now()


def manyapname(early_time):
    aplists = (i[0] for i in session.query(distinct(Unimac.ap_no_empty)).filter(Unimac.end_time>=early_time).all() if i[0])
    aset = set(aplists)
    return aset


def get_manu_flag(manyap_items):
    manu_flag = None
    if len(manyap_items) < 2:
        item = manyap_items[0]
        if item.flag:
            return None
        if 'UnKnow' in item.manu:
            manu_flag = '1IOS+' + str(hash(item.mac))
        elif 'Apple' in item.manu:
            manu_flag = 'IOS+' + str(hash(item.mac))
        return manu_flag

    for i in manyap_items:
        if 'UnKnow' in i.manu:
            continue
        elif 'Apple' in i.manu:
            manu_flag = 'IOS+' + str(hash(i.mac))
        else:
            return None

    if not manu_flag:
        manu_flag = '1IOS+' + str(hash(i.mac))

    return manu_flag


def getmidtime(start, end):
    startstamp = start.timestamp()
    endstamp = end.timestamp()

    midtime = time.strftime("%Y-%m-%d %H:%M:%S",
                            time.localtime((startstamp + endstamp) // 2))
    return midtime


def update_item(item_info):
    update_apname_flag = False
    mac = item_info.mac
    manu = item_info.manu
    station = item_info.station
    curren_time = item_info.reqtime
    apname = item_info.apname
    station = item_info.station
    
    item_info.newitem = False
 
    ten_min_before_time = curren_time - datetime.timedelta(minutes=10)
    ten_min_after_time = curren_time + datetime.timedelta(minutes=10)

    is_middle_time_item = session.query(Unimac).filter(and_(
        text("end_time>=:curren_time"), text("start_time<=:curren_time")
        ).params(curren_time=curren_time)).first()

    if is_middle_time_item:
        item = is_middle_time_item

        if apname:
            ap_set = item.get_eval("ap") | {apname}
            ap_no_empty_list = item.get_eval("ap_no_empty")
            if not ap_no_empty_list:
                ap_no_empty_list = [apname]
            elif apname not in ap_no_empty_list:
                bisect.insort(ap_no_empty_list, apname)

            item.ap = repr(ap_set)
            item.ap_no_empty = repr(ap_no_empty_list)
            session.commit()
         
    else:
        has_start_time_item = session.query(Unimac).filter(and_(
            text("end_time>=:ten_min_before_time"),text("end_time<=:curren_time"), Unimac.mac==mac, Unimac.station==station
            )).params(ten_min_before_time=ten_min_before_time, curren_time=curren_time).first()

        has_end_time_item = session.query(Unimac).filter(and_(
            text("start_time<:ten_min_after_time"), text("start_time>:curren_time"), Unimac.mac==mac, Unimac.station==station
            )).params(ten_min_after_time=ten_min_after_time, curren_time=curren_time).first()


        if has_start_time_item and has_end_time_item:
            start_time = has_start_time_item.start_time
            end_time = has_start_time_item.end_time
            
            ap_set = eval(has_start_time_item.ap) | eval(has_end_time_item.ap) | {apname}

            has_end_time_item.start_time = start_time
            has_end_time_item.mid_time = getmidtime(start_time, end_time)
            has_end_time_item.ap = repr(ap_set)
            has_end_time_item.ap_no_empty = repr(sorted(list(ap_set-{''})))

            session.delete(has_start_time_item)
            session.commit()

        elif has_start_time_item:
            item = has_start_time_item

            start_time = item.start_time
            end_time = curren_time

            if apname:
                ap_set = item.get_eval("ap") | {apname}
                ap_no_empty_list = item.get_eval("ap_no_empty")
                if not ap_no_empty_list:
                    ap_no_empty_list = [apname]
                elif apname not in ap_no_empty_list:
                    bisect.insort(ap_no_empty_list, apname)

                item.ap = repr(ap_set)
                item.ap_no_empty = repr(ap_no_empty_list)

                has_start_time_item.end_time = end_time
                has_start_time_item.mid_time = getmidtime(start_time, end_time)
                
                session.commit()

        elif has_end_time_item:
            item = has_end_time_item

            start_time = curren_time
            end_time = item.end_time

            if apname:
                ap_set = item.get_eval("ap") | {apname}
                ap_no_empty_list = item.get_eval("ap_no_empty")
                if not ap_no_empty_list:
                    ap_no_empty_list = [apname]
                elif apname not in ap_no_empty_list:
                    bisect.insort(ap_no_empty_list, apname)

                item.ap = repr(ap_set)
                item.ap_no_empty = repr(ap_no_empty_list)

                item.start_time = start_time
                item.mid_time = getmidtime(start_time, end_time)
                
                session.commit()

        else:
            item = Unimac();
            item.mac = mac
            item.manu = manu
            item.station = station
            item.start_time = curren_time
            item.end_time = curren_time
            item.mid_time = getmidtime(item.start_time, item.end_time)
            item.ap = repr({apname})
            if apname:
                item.ap_no_empty = repr([apname])
            else:
                item.ap_no_empty = None

            session.add(item)
            session.commit()
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
        list(map(update_item, session.query(Init_sta1).filter(Init_sta1.newitem == True).all()))

        interval = 24 * 2 # two day early
        early_time = datetime.datetime.now() - datetime.timedelta(hours=interval)
        unque_manyap_set = manyapname(early_time)

        for index in unque_manyap_set:
            manyap_items = session.query(Unimac).filter(and_(Unimac.ap>=early_time, Unimac.ap_no_empty == index)).all()
            flag = get_manu_flag(manyap_items)

            if flag:
                session.query(Unimac).filter(text("ap_no_empty=:index and end_time>=:early_time")).params(index=index, early_time=early_time).update({Unimac.flag : flag}, synchronize_session=False)
                session.commit()

        print('--------------')
        time.sleep(5)


if __name__ == "__main__":
    logger = log('macname.log')

    try:
        main()
    except Exception as e:
        session.rollback()
        session.close()
        logger.exception(e)


