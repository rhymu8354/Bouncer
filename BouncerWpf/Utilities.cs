using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Bouncer.Wpf {

    public static class Utilities {
        #region Public Methods

        public static string FormatDeltaTime(double seconds) {
            double days = seconds / 86400.0;
            int wholeDays = (int)Math.Floor(days);
            double fractionalDaySeconds = seconds - wholeDays * 86400.0;
            double hours = fractionalDaySeconds / 3600.0;
            int wholeHours = (int)Math.Floor(hours);
            double fractionalHourSeconds = fractionalDaySeconds - wholeHours * 3600.0;
            double minutes = fractionalHourSeconds / 60.0;
            int wholeMinutes = (int)Math.Floor(minutes);
            double remainderSeconds = fractionalHourSeconds - wholeMinutes * 60.0;
            int wholeRemainderSeconds = (int)Math.Floor(remainderSeconds);
            if (wholeDays > 0) {
                return String.Format(
                    "{0}d {1:00}:{2:00}:{3:00}",
                    wholeDays,
                    wholeHours,
                    wholeMinutes,
                    wholeRemainderSeconds
                );
            } else {
                return String.Format(
                    "{0:00}:{1:00}:{2:00}",
                    wholeHours,
                    wholeMinutes,
                    wholeRemainderSeconds
                );
            }
        }

        #endregion
    }

}
