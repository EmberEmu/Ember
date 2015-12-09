CREATE DATABASE `ember` /*!40100 DEFAULT CHARACTER SET utf8 */;
USE `ember`;

CREATE TABLE `users` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `username` varchar(255) NOT NULL,
  `s` varchar(255) NOT NULL,
  `v` varchar(255) NOT NULL,
  `creation_date` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `subscriber` bit(1) NOT NULL DEFAULT b'0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `username_UNIQUE` (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `realms` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(45) NOT NULL,
  `ip` varchar(45) NOT NULL,
  `type` int(11) NOT NULL,
  `flags` int(11) NOT NULL,
  `zone` int(11) NOT NULL,
  `population` float NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `bans` (
  `user_id` int(10) unsigned NOT NULL,
  `by_id` int(10) unsigned NOT NULL,
  `date` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `reason` longtext NOT NULL,
  PRIMARY KEY (`user_id`),
  KEY `banned_by_idx` (`by_id`),
  KEY `banned_user_idx` (`user_id`),
  CONSTRAINT `banned_by_id` FOREIGN KEY (`by_id`) REFERENCES `users` (`id`),
  CONSTRAINT `banned_user_id` FOREIGN KEY (`user_id`) REFERENCES `users` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `suspensions` (
  `user_id` int(10) unsigned NOT NULL,
  `by_id` int(10) unsigned NOT NULL,
  `start_date` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `end_date` datetime NOT NULL,
  `reason` longtext NOT NULL,
  PRIMARY KEY (`user_id`),
  KEY `suspended_by_idx` (`by_id`),
  CONSTRAINT `suspended_by` FOREIGN KEY (`by_id`) REFERENCES `users` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `suspended_user` FOREIGN KEY (`user_id`) REFERENCES `users` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `ip_bans` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `ip` varchar(45) NOT NULL,
  `cidr` int(11) NOT NULL,
  `reason` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;