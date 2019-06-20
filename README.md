# USTOR-64 - A Linux/C solution for Tracking UID's and Unique Capping.

### Initially intended for use with Advertising Servers or Real-Time Bidding (OpenRTB) Demand-Side Platforms (DSP).

**Performance**; READ O(1) - WRITE O(1) prime number hash map using CRC64.

**USTOR-64** uses the UDP protocol for maximum throughput, a hash map using a prime number, a simple bucketing system that is adequate enough for the purpose of dealing with hash collisions
and cache efficient pre-allocated memory.

**Threading**; POSIX Threads are used to thread the Read operations, one thread per CPU core is created.

**Read** operations are on UDP port 7811.
**Write** operations are single threaded on port 7810.

SHA1 is used in the php code to hash the userid before it is sent to the USTOR daemon, once it arrives it is hashed a second time  into a CRC64 index for the HashMap.

By default 433,033,301 *(433 Million)* impressions can be recorded not including collisions. Ccollisions are range blocked using two short integers to make the whole struct a total size of 8 bytes.

The current configuration uses ~3.2 GB of memory.


##### PHP Functions for Write and Check operations

```
function add_ustor($userid, $expire_seconds)
{
    if($expire_seconds == 0){$expire_seconds = 86400;}
    if($userid == ''){$userid = "u";}
    $fp = stream_socket_client("udp://127.0.0.1:7810", $errno, $errstr, 1);
    if($fp)
    {
        fwrite($fp, "$" . sha1($userid) . "|" . $expire_seconds);
        fclose($fp);
    }
}
```

```
function check_ustor($userid)
{
    if($userid == ''){$userid = "u";}
    $fp = stream_socket_client("udp://127.0.0.1:7811", $errno, $errstr, 1);
    if($fp)
    {
        $r = fwrite($fp, sha1($userid));
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
            return FALSE; //FALSE = Not Stored, You can bid.
        }
        fclose($fp);
        return TRUE;
    }
    return TRUE;
}
```

#### Licence
````
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
```
James William Flecher [github.com/mrbid] ~2018
public release.
