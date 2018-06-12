# Hik-UAV_Battle

HIKVISION CodeChallenge2018

UAV battles based on AI

Wish that I can go to the city and get the job which I desire.


------------------------------


## VERSIONS

##### date:		2018-05-25
##### version:		v1.0
##### description:
* For every effective good, calculate it's suitability with every free uav and find the most suitable one, then BFS the route.
* For every effective uav, if standby, then go to one standby point(BFS); if just catch good, then BFS again.
* There are only several floors need to BFS, and count total steps of every floor, then get the best one.

------------------------------
##### date:		2018-05-26
##### version:		v1.2
##### description:

**BFS STEP BY STEP !!!**

* Because of the long calculation process of BFS(due to the circle traverse), I cut the step, in other words, the radius of the circle traverse, into serveral steps. 
* As a result, the total time of one BFS is divided into several time point, hence the decrease of calculation pressure.
* Take good use of **time 0** to calculate some uavs' route in advance, meanwhile, share the pressure of **time 1**(cause it's the real start time point).
* According to the above, I also cut the first step length of **time 0** in order to decrease the pressure of first calculation.
* Change the init of map, and move the graph_init part to TaskInit so as to decrease the time of map_init.

------------------------------
##### date:		2018-05-27
##### version:		v1.3
##### description:
* Bubble sort the goods array from the most valuable to the least. And the most valuable has priority in the distribution(but considering the OMP, the effect is actually uncertain...).

------------------------------
##### date:		2018-05-28
##### version:		v1.4
##### description:
* Find the bug in CalculateSuitability():stblt has exceeded the maximum cause the original value is too little...
* Split BFS from distribution, so the good priority can really become effective~
* Drop **memset()** because of it's large time and space consuming, so the use of **memset()** should be as little as possible.
* Due to the drop of **memset()** in TaskInit(), the init of graphs has been moved into it's original place(MapInit()->GraphInit());
* It seems that BFS_STEP_LENGTH: 300 has the best effect in medium size map.

------------------------------
##### date:		2018-05-29
##### version:		v1.6
##### description:
* Fix the bug when catch the good.

------------------------------
##### date:		2018-05-30
##### version:		v1.8
##### description:
* Arrange the attack uavs which go to the enemy parking, and it's num is limited in 3.
* The route of attack uav needs to be optimized.
* Wish that I can go into the semi-finals.

------------------------------
##### date:		2018-06-07
##### version:		v2.0
##### description:
* Congratulations for entering the semi-finals!
* Add the electricity limit.
* Add the description of home parking state.
* Back home for charging when the remain_electricity is not full.

------------------------------
##### date:		2018-06-10
##### version:		v2.2
##### description:
* Fix some bugs when back home charge(so much...).
* If uav is attack one, then never change its aim unless it is destroyed.
* Change the purchase strategies: according to the goods(value, also the weight from much to little) which can be aimed, the find the matched uav(value, also the load_weight from little to much).
* **goods BFS's floors num has been cut to 1, in other words, the fly_low_limit floor.**

------------------------------
##### date:		2018-06-12
##### version:		v2.4
##### description:
* Already exhausted, even if I know that I need to improve.
* Long time coding and nobody to discuss makes me boring and even, disgusted.
* At the end of this competition, everything during it inspires me to doubt that if I am really suitable to be a software engineer?
* I find it seems that I am not very interested in total software works instead of I can't solve problems.
* Thanks to HikVision's competition for enlightening me so much, whether there is a prize or not.
* I need to embark on a new journey.^_^