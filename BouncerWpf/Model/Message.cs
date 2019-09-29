using Bouncer.Wpf.Properties;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace Bouncer.Wpf.Model {
    public class Message {

        #region Public Properties

        public uint Level { get; set; }
        public string Content { get; set; }
        public long Userid { get; set; }

        #endregion

        #region Public Methods

        public static Message StatusMessage(
            uint level,
            string content,
            long userid
        ) {
            var message = new Message();
            message.Level = level;
            message.Content = content;
            message.Userid = userid;
            return message;
        }

        #endregion

    }
}
