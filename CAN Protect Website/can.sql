-- phpMyAdmin SQL Dump
-- version 5.2.1
-- https://www.phpmyadmin.net/
--
-- Host: 127.0.0.1
-- Generation Time: Jun 07, 2025 at 05:06 AM
-- Server version: 10.4.32-MariaDB
-- PHP Version: 8.1.25

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `id_des`
--

-- --------------------------------------------------------

--
-- Table structure for table `can`
--

CREATE TABLE `can` (
  `id` int(11) NOT NULL,
  `model` enum('Standard','Extended') NOT NULL,
  `can_id` varchar(10) NOT NULL,
  `description` varchar(256) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `can`
--

INSERT INTO `can` (`id`, `model`, `can_id`, `description`) VALUES
(1, 'Standard', '1A3', 'Temperature'),
(2, 'Standard', '2BC', 'Velocity'),
(3, 'Extended', '1ABCDE0F', 'Blink LED'),
(4, 'Extended', '1C0FFEE0', 'Music'),
(5, 'Standard', '3F5', 'Battery Percent'),
(6, 'Standard', '1F5', 'Engine Status');

--
-- Indexes for dumped tables
--

--
-- Indexes for table `can`
--
ALTER TABLE `can`
  ADD PRIMARY KEY (`id`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `can`
--
ALTER TABLE `can`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=8;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
