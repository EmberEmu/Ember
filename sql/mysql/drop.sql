REVOKE ALL PRIVILEGES ON * . * FROM 'ember'@'localhost';

REVOKE ALL PRIVILEGES ON `ember` . * FROM 'ember'@'localhost';

REVOKE GRANT OPTION ON `ember` . * FROM 'ember'@'localhost';

DELETE FROM `user` WHERE CONVERT( User USING utf8 ) = CONVERT( 'ember' USING utf8 ) AND CONVERT( Host USING utf8 ) = CONVERT( 'localhost' USING utf8 ) ;

DROP DATABASE IF EXISTS `ember` ;
