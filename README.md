# USTOR-64 - A Linux/C solution for Tracking UID's and Unique Capping.

### Initially intended for use with Advertising Servers or Real-Time Bidding (OpenRTB) Demand-Side Platforms (DSP).

*Term: IDFA - a unique user id*

Performance; READ O(1) - WRITE O(1) prime number hash map using CRC64.

This version of USTOR uses the UDP protocol for maximum throughput,
a hash map using a prime number, a simple bucketing system that is
adequate enough for the purpose of dealing with hash collisions
and cache efficient pre-allocated memory.

POSIX Threads are used to thread the Read operations, on a separate
port with port sharing enabled. (port 7811)

Write operations are single threaded. (port 7810)

SHA1 is used in the php code to hash the idfa before it is sent to
the USTOR daemon, once it arrives it is hashed using the CRC64
algorithm from Redis. Then it is added to the hashmap.

By default 433,033,301 (433 Million) impressions can be recorded
not including collisions, which from studying the code you will find
collisions are range blocked using two short integers to make the whole
struct a total size of 8 bytes.

The current configuration uses ~3.2 GB of memory.


##### PHP Functions for Write and Check operations

```
function add_ustor($idfa, $expire_seconds)
{
    if($expire_seconds == 0){$expire_seconds = 86400;}
    if($idfa == ''){$idfa = "u";}
    $fp = stream_socket_client("udp://127.0.0.1:7810", $errno, $errstr, 1);
    if($fp)
    {
        fwrite($fp, "$" . sha1($idfa) . "|" . $expire_seconds);
        fclose($fp);
    }
}
```

```
function check_ustor($idfa)
{
    if($idfa == ''){$idfa = "u";}
    $fp = stream_socket_client("udp://127.0.0.1:7811", $errno, $errstr, 1);
    if($fp)
    {
        $r = fwrite($fp, sha1($idfa));
        if($r == FALSE)
        {
            fclose($fp);
            return TRUE;
        }
        //stream_set_timeout($fp, 1);
        $r = fread($fp, 1);
        if($r != FALSE && $r[0] == 'n')
        {
            fclose($fp);
            return FALSE; //FASLE = Not Stored, You can bid.
        }
        fclose($fp);
        return TRUE;
    }
    return TRUE;
}
```

>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

James William Flecher [github.com/mrbid] ~2018
public release.
